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

#ifndef UPDATERBACKEND_H
#define UPDATERBACKEND_H
#include <QStringList>
#include <QObject>
#include <QDBusInterface>
#include "zypperWrapper.h"

class UpdaterBackend : public QObject
{
	Q_OBJECT

    private:
		ZypperWrapper *wrapper;
		QDBusInterface *dbusifScreenSaver {nullptr};
		QDBusInterface *dbusifPower {nullptr};
		uint32_t cookiescreensaver {0};
		uint32_t cookiepower {0};
		bool updatesNotificationShowed;
		void inhibitor(bool inhibit);
				
	public:
		explicit UpdaterBackend(QObject *parent = nullptr);
		~UpdaterBackend();
		
	public slots:
// 		Q_INVOKABLE bool pendingStatus() const;
		Q_INVOKABLE void checkUpdates();
		Q_INVOKABLE void installUpdates();
		Q_INVOKABLE void installOptions(bool autoresolveConflicts, bool autoagreeLicenses);
		Q_INVOKABLE QVariantList listCheckUpdates();
		Q_INVOKABLE void promptInput(const QString inputStr);
		
	signals:
		void sendCheckUpdates(bool checked = false);
		void sendInstallUpdates();
		void sendInstallOptions(bool autoresolveConflicts, bool autoagreeLicenses);
		void sendPromptInput(const QString inputStr);
		void checkCompleted(bool install);
		void installCompleted();
		void installPrompt(QVariantList promptParams);
		void installMessage(QVariantList messageParams);
		void operationAborted(int abortType);
		void installResumed(int numPackages);
		void headerMessage(QString messageText);

	private slots:
		void checkCompletedWrapperReceived(bool install = false);
		void installCompletedWrapperReceived();
		void installPromptWrapperReceived(QVariantList promptoptlist);
		void installMessageWrapperReceived(QVariantList messagelist);
		void operationAbortedWrapperReceived(int aborttype);
		void installResumedWrapperReceived(int numpackages);
		void headerMessageWrapperReceived(QString messagetext);

};

#endif // UPDATERBACKEND_H
