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
	dbusifScreenSaver = new QDBusInterface("org.freedesktop.ScreenSaver", "/ScreenSaver", "org.freedesktop.ScreenSaver", QDBusConnection::sessionBus());
	dbusifPower = new QDBusInterface("org.freedesktop.PowerManagement.Inhibit", "/org/freedesktop/PowerManagement", "org.freedesktop.PowerManagement.Inhibit", QDBusConnection::sessionBus());
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
	emit UpdaterBackend::sendCheckUpdates();
}

Q_INVOKABLE void UpdaterBackend::installUpdates()
{
	if(wrapper->isProcessRunning())
		return;
	inhibitor(true);
	emit UpdaterBackend::sendInstallUpdates();
}

Q_INVOKABLE void UpdaterBackend::installOptions(bool autoresolveConflicts, bool autoagreeLicenses)
{
	emit UpdaterBackend::sendInstallOptions(autoresolveConflicts, autoagreeLicenses);
}

Q_INVOKABLE QVariantList UpdaterBackend::listCheckUpdates()
{
// 	qDebug() << "listCheckUpdates " << QTime::currentTime().toString() << " " << wrapper->updatesList.size();
	return wrapper->updatesList;
}

Q_INVOKABLE void UpdaterBackend::promptInput(const QString inputStr)
{
// 	qDebug() << "promptInput " << QTime::currentTime().toString() << " " << inputStr;
	emit UpdaterBackend::sendPromptInput(inputStr);
}

void UpdaterBackend::inhibitor(bool inhibit)
{
	if (inhibit) {
		if (dbusifScreenSaver->isValid()) {
			cookiescreensaver = 0;
            QDBusMessage msgreply = dbusifScreenSaver->call("Inhibit", "com.contezero.twupdater", "Installing Updates");
            if (msgreply.type() == QDBusMessage::ReplyMessage) {
                QList<QVariant> replylist = msgreply.arguments();
                QVariant reply = replylist.first();
                cookiescreensaver = reply.toUInt();
            }
		}
		if (dbusifPower->isValid()) {
			cookiepower = 0;
            QDBusMessage msgreply = dbusifPower->call("Inhibit", "com.contezero.twupdater", "Installing Updates");
            if (msgreply.type() == QDBusMessage::ReplyMessage) {
                QList<QVariant> replylist = msgreply.arguments();
                QVariant reply = replylist.first();
                cookiepower = reply.toUInt();
            }
		}
	} else {
		if (dbusifScreenSaver->isValid()) {
            if (cookiescreensaver != 0) {
                dbusifScreenSaver->call("UnInhibit" , cookiescreensaver);
                cookiescreensaver = 0;
            }
		}
		if (dbusifPower->isValid()) {
            if (cookiepower != 0) {
                dbusifPower->call("UnInhibit" , cookiepower);
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
		if (updatesText == "") {
		//TODO: add translations support
			KNotification::event(KNotification::Notification, "Tumbleweed Updater", "Available Updates: " + QString::number(wrapper->getNumpackages()), "update-low", 0, KNotification::Persistent);
		} else {
			KNotification::event(KNotification::Notification, "Tumbleweed Updater", updatesText, "update-low", 0, KNotification::Persistent);
		}
	}
	emit UpdaterBackend::checkCompleted(install);
}

void UpdaterBackend::installCompletedWrapperReceived()
{
// 	qDebug() << "installCompletedWrapperReceived " << QTime::currentTime().toString();
	inhibitor(false);
	updatesNotificationShowed = false;
	QString installText = wrapper->getNotificationText(2);
	if (installText == "") {
		//TODO: add translations support
		KNotification::event(KNotification::Notification, "Tumbleweed Updater", "Installation Completed", "update-none", 0, KNotification::Persistent);
	} else {
		KNotification::event(KNotification::Notification, "Tumbleweed Updater", installText, "update-none", 0, KNotification::Persistent);
	}
	emit UpdaterBackend::installCompleted();
}

void UpdaterBackend::installPromptWrapperReceived(QVariantList promptoptlist)
{
// 	qDebug() << "installPromptWrapperReceived " << QTime::currentTime().toString() << promptoptlist.length();
	emit UpdaterBackend::installPrompt(promptoptlist);
}

void UpdaterBackend::installMessageWrapperReceived(QVariantList messagelist)
{
// 	qDebug() << "installMessageWrapperReceived " << QTime::currentTime().toString() << messagelist.length();
	emit UpdaterBackend::installMessage(messagelist);
}

void UpdaterBackend::operationAbortedWrapperReceived(int aborttype)
{
// 	qDebug() << "operationAbortedWrapperReceived " << QTime::currentTime().toString() << QString::number(aborttype);
	if ((aborttype == 0) || (aborttype == 2)) {
		inhibitor(false);
	} else if (aborttype == 10) {
		QString abortText = wrapper->getNotificationText(10);
		KNotification::event(KNotification::Notification, "Tumbleweed Updater", abortText, "update-none", 0, KNotification::Persistent);
	}
	emit UpdaterBackend::operationAborted(aborttype);
}

void UpdaterBackend::installResumedWrapperReceived(int numpackages)
{
// 	qDebug() << "installResumedWrapperReceived " << QTime::currentTime().toString() << numpackages;
	emit UpdaterBackend::installResumed(numpackages);
}

void UpdaterBackend::headerMessageWrapperReceived(QString messagetext)
{
// 	qDebug() << "headerMessageWrapperReceived " << QTime::currentTime().toString() << messagetext;
	emit UpdaterBackend::headerMessage(messagetext);
}
