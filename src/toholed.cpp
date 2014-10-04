#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>
#include "toholed.h"
#include "toh.h"
#include "oled.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QStringList>
#include <QtDBus/QtDBus>
#include <QDBusConnection>
#include <QDBusMessage>

#include "toholed-dbus.h"

#include "notificationmanager.h"

#include <QtGlobal>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    daemonize();

    setlinebuf(stdout);
    setlinebuf(stderr);

    printf("Starting toholed daemon. Version %s.\n", APPVERSION);

    if (!QDBusConnection::systemBus().isConnected())
    {
        printf("Cannot connect to the D-Bus systemBus\n%s\n", qPrintable(QDBusConnection::systemBus().lastError().message()));
        exit(EXIT_FAILURE);
    }
    printf("Connected to D-Bus systembus\n");

    printf("Environment %s\n", qPrintable(getenv ("DBUS_SESSION_BUS_ADDRESS")));

    if (!QDBusConnection::sessionBus().isConnected())
    {
        printf("Cannot connect to the D-Bus sessionBus\n%s\n", qPrintable(QDBusConnection::sessionBus().lastError().message()));
        exit(EXIT_FAILURE);
    }
    printf("Connected to D-Bus sessionbus\n");

    if (!QDBusConnection::systemBus().registerService(SERVICE_NAME))
    {
        printf("Cannot register service to systemBus\n%s\n", qPrintable(QDBusConnection::systemBus().lastError().message()));
        exit(EXIT_FAILURE);
    }


    printf("Registered %s to D-Bus systembus\n", SERVICE_NAME);

    Toholed toholed;

    QDBusConnection::systemBus().registerObject("/", &toholed, QDBusConnection::ExportAllSlots);

    /* Ofono MessageManager IncomingMessage
     * This signal is emitted when new SMS message arrives */

    static QDBusConnection ofonoSMSconn = QDBusConnection::systemBus();
    ofonoSMSconn.connect("org.ofono", "/ril_0", "org.ofono.MessageManager", "IncomingMessage",
                      &toholed, SLOT(handleSMS(const QDBusMessage&)));

    if(ofonoSMSconn.isConnected())
        printf("Ofono.MessageManager.IncomingMessage Connected\n");
    else
        printf("Ofono.MessageManager.IncomingMessage Not connected\n%s\n", qPrintable(ofonoSMSconn.lastError().message()));


    /* path=/com/nokia/mce/signal; interface=com.nokia.mce.signal; member=sig_call_state_ind */

    static QDBusConnection mceCallStateconn = QDBusConnection::systemBus();
    mceCallStateconn.connect("com.nokia.mce", "/com/nokia/mce/signal", "com.nokia.mce.signal", "sig_call_state_ind",
                          &toholed, SLOT(handleCall(const QDBusMessage&)));

    if(mceCallStateconn.isConnected())
        printf("com.nokia.mce.signal.sig_call_state_ind Connected\n");
    else
        printf("com.nokia.mce.signal.sig_call_state_ind Not connected\n%s\n", qPrintable(mceCallStateconn.lastError().message()));


    /* Nokia MCE display_status_ind
     * No actual use with this, just make log entry. Display status returns string: "on", "dimmed" or "off"  */

    static QDBusConnection mceDisplayStatusconn = QDBusConnection::systemBus();
    mceDisplayStatusconn.connect("com.nokia.mce", "/com/nokia/mce/signal", "com.nokia.mce.signal", "display_status_ind",
                          &toholed, SLOT(handleDisplayStatus(const QDBusMessage&)));

    if(mceDisplayStatusconn.isConnected())
        printf("com.nokia.mce.signal.display_status_ind Connected\n");
    else
        printf("com.nokia.mce.signal.display_status_ind Not connected\n%s\n", qPrintable(mceDisplayStatusconn.lastError().message()));

    /* Freedesktop Notifications NotificationClosed
     * This signal is emitted when notification is closed. We can then remove icons from screen */

    static QDBusConnection freeNotifconn = QDBusConnection::sessionBus();
    freeNotifconn.connect("org.freedesktop.Notifications", "/org/freedesktop/Notifications", "org.freedesktop.Notifications", "NotificationClosed",
                          &toholed, SLOT(handleNotificationClosed(const QDBusMessage&)));

    freeNotifconn.connect("org.freedesktop.Notifications", "/org/freedesktop/Notifications", "org.freedesktop.Notifications", "ActionInvoked",
                          &toholed, SLOT(handleNotificationActionInvoked(const QDBusMessage&)));

    if(freeNotifconn.isConnected())
        printf("freedesktop.Notifications.NotificationClosed Connected\n");
    else
        printf("freedesktop.Notifications.NotificationClosed Not connected\n%s\n", qPrintable(freeNotifconn.lastError().message()));



    /* path=/com/tweetian; com.tweetian member=newNotification  */

    static QDBusConnection tweetianConn = QDBusConnection::sessionBus();
    tweetianConn.connect("com.tweetian", "/com/tweetian", "com.tweetian", "newNotification",
                          &toholed, SLOT(handleTweetian(const QDBusMessage&)));

    if(tweetianConn.isConnected())
        printf("com.tweetian.newNotification Connected\n");
    else
        printf("com.tweetian.newNotification Not connected\n%s\n", qPrintable(tweetianConn.lastError().message()));


    /* Communi IRC connection */

    static QDBusConnection communiConn = QDBusConnection::sessionBus();
    communiConn.connect("com.communi.irc", "/", "com.communi.irc", "activeHighlightsChanged",
                          &toholed, SLOT(handleCommuni(const QDBusMessage&)));

    if(communiConn.isConnected())
        printf("com.communi.irc.highlightedSimple Connected\n");
    else
        printf("com.communi.irc.highlightedSimple Connected\n%s\n", qPrintable(communiConn.lastError().message()));


    /* Charger connected/disconnected */

    static QDBusConnection chargerConnectionconn = QDBusConnection::systemBus();
    chargerConnectionconn.connect("com.meego.usb_moded", "/com/meego/usb_moded", "com.meego.usb_moded", "sig_usb_state_ind",
                          &toholed, SLOT(handleChargerStatus(const QDBusMessage&)));

    if(chargerConnectionconn.isConnected())
        printf("com.meego.usb_moded.sig_usb_state_ind Connected\n");
    else
        printf("com.meego.usb_moded.sig_usb_state_ind Not connected\n%s\n", qPrintable(chargerConnectionconn.lastError().message()));


    /* Mitäkuuluu unread */

    static QDBusConnection mitakuuluuConn = QDBusConnection::sessionBus();
    mitakuuluuConn.connect("harbour.mitakuuluu2.client", "/", "harbour.mitakuuluu2.client", "totalUnreadValue",
                           &toholed, SLOT(handleMitakuuluu(const QDBusMessage&)));

    if (mitakuuluuConn.isConnected())
        printf("harbour.mitakuuluu2.client totalUnreadValue Connected\n");
    else
        printf("harbour.mitakuuluu2.client totalUnreadValue Not connected\n%s\n", qPrintable(mitakuuluuConn.lastError().message()));


    /* Silent profile */

    static QDBusConnection profileChangedConn = QDBusConnection::sessionBus();
    profileChangedConn.connect("com.nokia.profiled", "/com/nokia/profiled", "com.nokia.profiled", "profile_changed",
                           &toholed, SLOT(handleProfileChanged(const QDBusMessage&)));

    if (profileChangedConn.isConnected())
        printf("com.nokia.profiled profile_changed Connected\n");
    else
        printf("com.nokia.profiled profile_changed Not connected\n%s\n", qPrintable(profileChangedConn.lastError().message()));

    /* Alarms */

    static QDBusConnection alarmConn = QDBusConnection::systemBus();
    alarmConn.connect("com.nokia.voland.signal", "/com/nokia/voland/signal", "com.nokia.voland.signal", "visual_reminders_status",
                           &toholed, SLOT(handleAlarm(const QDBusMessage&)));

    if (alarmConn.isConnected())
        printf("com.nokia.voland.signal visual_reminders_status Connected\n");
    else
        printf("com.nokia.voland.signal visual_reminders_status Not connected\n%s\n", qPrintable(alarmConn.lastError().message()));

    /* Network technology */

    // path=/ril_0; interface=org.ofono.NetworkRegistration; member=PropertyChanged

    static QDBusConnection networkRegConn = QDBusConnection::systemBus();
    networkRegConn.connect("org.ofono", "/ril_0", "org.ofono.NetworkRegistration", "PropertyChanged",
                           &toholed, SLOT(handleNetworkRegistration(const QDBusMessage&)));

    if (networkRegConn.isConnected())
        printf("org.ofono.NetworkRegistration PropertyChanged Connected\n");
    else
        printf("org.ofono.NetworkRegistration PropertyChanged Not connected\n%s\n", qPrintable(networkRegConn.lastError().message()));

    /* bluetooth */

    static QDBusConnection bluetoothConn = QDBusConnection::systemBus();
    bluetoothConn.connect("net.connman", "/net/connman/technology/bluetooth", "net.connman.Technology", "PropertyChanged",
                           &toholed, SLOT(handleBluetooth(const QDBusMessage&)));

    if (bluetoothConn.isConnected())
        printf("net.connman.Technology bluetooth PropertyChanged Connected\n");
    else
        printf("net.connman.Technology bluetooth PropertyChanged PropertyChanged Not connected\n%s\n", qPrintable(bluetoothConn.lastError().message()));

    /* wifi */

    static QDBusConnection wifiConn = QDBusConnection::systemBus();
    wifiConn.connect("net.connman", "/net/connman/technology/wifi", "net.connman.Technology", "PropertyChanged",
                           &toholed, SLOT(handleWifi(const QDBusMessage&)));

    if (wifiConn.isConnected())
        printf("net.connman.Technology wifi PropertyChanged Connected\n");
    else
        printf("net.connman.Technology wifi PropertyChanged PropertyChanged Not connected\n%s\n", qPrintable(wifiConn.lastError().message()));


    /* cellular */

    static QDBusConnection cellularConn = QDBusConnection::systemBus();
    cellularConn.connect("net.connman", "/net/connman/technology/cellular", "net.connman.Technology", "PropertyChanged",
                           &toholed, SLOT(handleCellular(const QDBusMessage&)));

    if (cellularConn.isConnected())
        printf("net.connman.Technology cellular PropertyChanged Connected\n");
    else
        printf("net.connman.Technology cellular PropertyChanged PropertyChanged Not connected\n%s\n", qPrintable(cellularConn.lastError().message()));


    NotificationManager notifications;

    notifications.connect(&notifications, SIGNAL(emailNotify()), &toholed, SLOT(handleEmailNotify()));
    notifications.connect(&notifications, SIGNAL(twitterNotify()), &toholed, SLOT(handleTwitterNotify()));
    notifications.connect(&notifications, SIGNAL(facebookNotify()), &toholed, SLOT(handleFacebookNotify()));
    notifications.connect(&notifications, SIGNAL(irssiNotify()), &toholed, SLOT(handleIrssiNotify()));
    notifications.connect(&notifications, SIGNAL(imNotify()), &toholed, SLOT(handleImNotify()));
    notifications.connect(&notifications, SIGNAL(otherNotify()), &toholed, SLOT(handleOtherNotify()));

    return app.exec();

}


void daemonize()
{
	/* Change the file mode mask */
	umask(0);

	/* Change the current working directory */
	if ((chdir("/tmp")) < 0) 
		exit(EXIT_FAILURE);

	/* register signals to monitor / ignore */
	signal(SIGCHLD,SIG_IGN); /* ignore child */
	signal(SIGTSTP,SIG_IGN); /* ignore tty signals */
	signal(SIGTTOU,SIG_IGN);
	signal(SIGTTIN,SIG_IGN);
	signal(SIGHUP,signalHandler); /* catch hangup signal */
	signal(SIGTERM,signalHandler); /* catch kill signal */

    qInstallMessageHandler(myMessageOutput);
}


void signalHandler(int sig) /* signal handler function */
{
	switch(sig)
	{
		case SIGHUP:
			/* rehash the server */
            printf("Received signal SIGHUP\n");
			break;		
		case SIGTERM:
			/* finalize the server */
            printf("Received signal SIGTERM\n");
            deinitOled();
            controlVdd(0);
			exit(0);
			break;		
	}	
}


void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    switch (type) {
    case QtDebugMsg:
        fprintf(stderr, "Debug: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtWarningMsg:
        fprintf(stderr, "Warning: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtCriticalMsg:
        fprintf(stderr, "Critical: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtFatalMsg:
        fprintf(stderr, "Fatal: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    }
}
