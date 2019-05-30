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

#ifndef ZYPPERWRAPPER_H
#define ZYPPERWRAPPER_H

#include <QString>
#include <QStringList>
#include <QByteArray>
#include <QProcess>
#include <QXmlStreamReader>
#include <QTimer>

class ZypperWrapper : public QObject
{
	Q_OBJECT
	
	private:
		QProcess *wrapperProcess;
		QXmlStreamReader *xmlstreamreader;
		QTimer *aliveTimer;
		QTimer *retrycheckTimer;
		QByteArray xmlstream;
		QByteArray summarystream;
		int packageTo;
		int numpackages;
		int numerrorpromptcheck;
		int resumeType;
		QString xmlexitcode;
		bool autoResolveConflicts;
		bool autoAgreeLicenses;
		bool xmlstreamprompt;
		int parseStep;
		QStringList xmlmessageList;
		QStringList xmlmessageErrorList;
		QStringList xmlmessageInstallCompletedList;
		QVariantList promptOptList;
		QString promptId;
		QString availableUpdatesText;
		QString installCompletedText;
		int processRunning;
		bool parsingSummary;
		bool firstCheck;
		QList<QString> inputValues;
				
		void parseXmlUpdateList();
		void parseXmlInput();
		void resetVars();
		void installProcessFinished();
		void checkProcessFinished();
		void searchUpdatesText();
				
	public:
		explicit ZypperWrapper(QObject *parent = nullptr);
		~ZypperWrapper();
		bool isProcessRunning();
		int getNumpackages();
		QString getNotificationText(int type);
		QVariantList updatesList;

	signals:
		void checkCompletedWrapper(bool install = false);
		void installCompletedWrapper();
		void installPromptWrapper(QVariantList promptoptlist);
		void installMessageWrapper(QVariantList messagelist);
		void operationAbortedWrapper(int aborttype);
		void installResumedWrapper(int numpackages);
		void headerMessageWrapper(QString messagetext);
		
	private slots:
		void checkUpdatesOutput();
		void checkUpdatesFinished(int exitCode, QProcess::ExitStatus exitStatus);
		void checkUpdatesError(QProcess::ProcessError error);
		void installUpdatesOutput();
		void installUpdatesFinished(int exitCode, QProcess::ExitStatus exitStatus);
		void installUpdatesError(QProcess::ProcessError error);
		void tryResumeFinished(int exitCode, QProcess::ExitStatus exitStatus);
		void tryResumeError(QProcess::ProcessError error);
		void retryCheck();
		void isSessionAlive();
		void isSessionAliveFinished(int exitCode, QProcess::ExitStatus exitStatus);
	
	public slots:
		void checkUpdatesStart(bool checked = false);
		void installUpdatesStart();
		void setInstallOptions(bool autoresolveConflicts, bool autoagreeLicenses);
		void promptInputWrite(const QString inputStr);

};

#endif // ZYPPERWRAPPER_H
