/*
 *   Copyright (C) 2019 by Michele Cherici <contact@contezero.com> 
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "updaterBackend.h"
#include <QString>
#include <KNotification>
#include <QDBusConnection>
// #include <QDebug>
// #include <QTime>


UpdaterBackend::UpdaterBackend(QObject *parent) : QObject(parent)
{
	wrapper = new ZypperWrapper;
	connect(this, &UpdaterBackend::sendCheckUpdates, wrapper, &ZypperWrapper::checkUpdatesStart, (Qt::ConnectionType)(Qt::UniqueConnection | Qt::DirectConnection));
	connect(this, &UpdaterBackend::sendInstallUpdates, wrapper, &ZypperWrapper::installUpdatesStart, (Qt::ConnectionType)(Qt::UniqueConnection | Qt::DirectConnection));
	connect(this, &UpdaterBackend::sendInstallOptions, wrapper, &ZypperWrapper::setInstallOptions, (Qt::ConnectionType)(Qt::UniqueConnection | Qt::DirectConnection));
	connect(this, &UpdaterBackend::sendPromptInput, wrapper, &ZypperWrapper::promptInputWrite, (Qt::ConnectionType)(Qt::UniqueConnection | Qt::DirectConnection));
	connect(wrapper, &ZypperWrapper::checkCompletedWrapper, this, &UpdaterBackend::checkCompletedWrapperReceived, (Qt::ConnectionType)(Qt::UniqueConnection | Qt::DirectConnection));
	connect(wrapper, &ZypperWrapper::installCompletedWrapper, this, &UpdaterBackend::installCompletedWrapperReceived, (Qt::ConnectionType)(Qt::UniqueConnection | Qt::DirectConnection));
	connect(wrapper, &ZypperWrapper::installPromptWrapper, this, &UpdaterBackend::installPromptWrapperReceived, (Qt::ConnectionType)(Qt::UniqueConnection | Qt::DirectConnection));
	connect(wrapper, &ZypperWrapper::installMessageWrapper, this, &UpdaterBackend::installMessageWrapperReceived, (Qt::ConnectionType)(Qt::UniqueConnection | Qt::DirectConnection));
	connect(wrapper, &ZypperWrapper::operationAbortedWrapper, this, &UpdaterBackend::operationAbortedWrapperReceived, (Qt::ConnectionType)(Qt::UniqueConnection | Qt::DirectConnection));
	connect(wrapper, &ZypperWrapper::installResumedWrapper, this, &UpdaterBackend::installResumedWrapperReceived, (Qt::ConnectionType)(Qt::UniqueConnection | Qt::DirectConnection));
	connect(wrapper, &ZypperWrapper::headerMessageWrapper, this, &UpdaterBackend::headerMessageWrapperReceived, (Qt::ConnectionType)(Qt::UniqueConnection | Qt::DirectConnection));
	dbusifScreenSaver = new QDBusInterface(QStringLiteral("org.freedesktop.ScreenSaver"), QStringLiteral("/ScreenSaver"), QStringLiteral("org.freedesktop.ScreenSaver"), QDBusConnection::sessionBus());
	dbusifPower = new QDBusInterface(QStringLiteral("org.freedesktop.PowerManagement.Inhibit"), QStringLiteral("/org/freedesktop/PowerManagement"), QStringLiteral("org.freedesktop.PowerManagement.Inhibit"), QDBusConnection::sessionBus());
	updatesNotificationShowed = false;
// 	qDebug() << "UpdaterBackend Start " << QTime::currentTime().toString() << endl;
}

UpdaterBackend::~UpdaterBackend()
{
// 	delete dbusifScreenSaver;
// 	qDebug() << "UpdaterBackend Stop " << QTime::currentTime().toString() << endl;
}

// Q_INVOKABLE bool UpdaterBackend::pendingStatus() const
// {
//     if (wrapper->isProcessRunning()) 
// 		return true;
// 	return false;
// }

Q_INVOKABLE void UpdaterBackend::checkUpdates()
{
	if(wrapper->isProcessRunning())
		return;
	Q_EMIT UpdaterBackend::sendCheckUpdates();
}

Q_INVOKABLE void UpdaterBackend::installUpdates()
{
	if(wrapper->isProcessRunning())
		return;
	inhibitor(true);
	Q_EMIT UpdaterBackend::sendInstallUpdates();
}

Q_INVOKABLE void UpdaterBackend::installOptions(bool autoresolveConflicts, bool autoagreeLicenses, bool enableLogging)
{
	Q_EMIT UpdaterBackend::sendInstallOptions(autoresolveConflicts, autoagreeLicenses, enableLogging);
}

Q_INVOKABLE QVariantList UpdaterBackend::listCheckUpdates()
{
// 	qDebug() << "listCheckUpdates " << QTime::currentTime().toString() << " " << wrapper->updatesList.size();
	return wrapper->updatesList;
}

Q_INVOKABLE void UpdaterBackend::promptInput(const QString inputStr)
{
// 	qDebug() << "promptInput " << QTime::currentTime().toString() << " " << inputStr;
	Q_EMIT UpdaterBackend::sendPromptInput(inputStr);
}

void UpdaterBackend::inhibitor(bool inhibit)
{
	if (inhibit) {
		if (dbusifScreenSaver->isValid()) {
			cookiescreensaver = 0;
            QDBusMessage msgreply = dbusifScreenSaver->call(QStringLiteral("Inhibit"), QStringLiteral("com.contezero.twupdater"), QStringLiteral("Installing Updates"));
            if (msgreply.type() == QDBusMessage::ReplyMessage) {
                QList<QVariant> replylist = msgreply.arguments();
                QVariant reply = replylist.first();
                cookiescreensaver = reply.toUInt();
            }
		}
		if (dbusifPower->isValid()) {
			cookiepower = 0;
            QDBusMessage msgreply = dbusifPower->call(QStringLiteral("Inhibit"), QStringLiteral("com.contezero.twupdater"), QStringLiteral("Installing Updates"));
            if (msgreply.type() == QDBusMessage::ReplyMessage) {
                QList<QVariant> replylist = msgreply.arguments();
                QVariant reply = replylist.first();
                cookiepower = reply.toUInt();
            }
		}
	} else {
		if (dbusifScreenSaver->isValid()) {
            if (cookiescreensaver != 0) {
                dbusifScreenSaver->call(QStringLiteral("UnInhibit"), cookiescreensaver);
                cookiescreensaver = 0;
            }
		}
		if (dbusifPower->isValid()) {
            if (cookiepower != 0) {
                dbusifPower->call(QStringLiteral("UnInhibit"), cookiepower);
                cookiepower = 0;
            }
		}
	}
}

void UpdaterBackend::checkCompletedWrapperReceived(bool install)
{
// 	qDebug() << "checkCompletedWrapperReceived " << QTime::currentTime().toString() << " " << install;
	if (!install && !updatesNotificationShowed && (wrapper->getNumpackages() > 0)) {
		updatesNotificationShowed = true;
		QString updatesText = wrapper->getNotificationText(1);
		if (updatesText == QStringLiteral("")) {
		//TODO: add translations support
			KNotification::event(KNotification::Notification, QStringLiteral("Tumbleweed Updater"), QStringLiteral("Available Updates: ") + QString::number(wrapper->getNumpackages()), QStringLiteral("update-low"), KNotification::Persistent);
		} else {
			KNotification::event(KNotification::Notification, QStringLiteral("Tumbleweed Updater"), updatesText, QStringLiteral("update-low"), KNotification::Persistent);
		}
	}
	Q_EMIT UpdaterBackend::checkCompleted(install);
}

void UpdaterBackend::installCompletedWrapperReceived()
{
// 	qDebug() << "installCompletedWrapperReceived " << QTime::currentTime().toString();
	inhibitor(false);
	updatesNotificationShowed = false;
	QString installText = wrapper->getNotificationText(2);
	if (installText == QStringLiteral("")) {
		//TODO: add translations support
		KNotification::event(KNotification::Notification, QStringLiteral("Tumbleweed Updater"), QStringLiteral("Installation Completed"), QStringLiteral("update-none"), KNotification::Persistent);
	} else {
		KNotification::event(KNotification::Notification, QStringLiteral("Tumbleweed Updater"), installText, QStringLiteral("update-none"), KNotification::Persistent);
	}
	Q_EMIT UpdaterBackend::installCompleted();
}

void UpdaterBackend::installPromptWrapperReceived(QVariantList promptoptlist)
{
// 	qDebug() << "installPromptWrapperReceived " << QTime::currentTime().toString() << promptoptlist.length();
	Q_EMIT UpdaterBackend::installPrompt(promptoptlist);
}

void UpdaterBackend::installMessageWrapperReceived(QVariantList messagelist)
{
// 	qDebug() << "installMessageWrapperReceived " << QTime::currentTime().toString() << messagelist.length();
	Q_EMIT UpdaterBackend::installMessage(messagelist);
}

void UpdaterBackend::operationAbortedWrapperReceived(int aborttype)
{
// 	qDebug() << "operationAbortedWrapperReceived " << QTime::currentTime().toString() << QString::number(aborttype);
	if ((aborttype == 0) || (aborttype == 2)) {
		inhibitor(false);
	} else if (aborttype == 10) {
		QString abortText = wrapper->getNotificationText(10);
		KNotification::event(KNotification::Notification, QStringLiteral("Tumbleweed Updater"), abortText, QStringLiteral("update-none"), KNotification::Persistent);
	}
	Q_EMIT UpdaterBackend::operationAborted(aborttype);
}

void UpdaterBackend::installResumedWrapperReceived(int numpackages)
{
// 	qDebug() << "installResumedWrapperReceived " << QTime::currentTime().toString() << numpackages;
	Q_EMIT UpdaterBackend::installResumed(numpackages);
}

void UpdaterBackend::headerMessageWrapperReceived(QString messagetext)
{
// 	qDebug() << "headerMessageWrapperReceived " << QTime::currentTime().toString() << messagetext;
	Q_EMIT UpdaterBackend::headerMessage(messagetext);
}
