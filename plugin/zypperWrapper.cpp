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
#include "zypperWrapper.h"
#include <QFile>
#include <QRegularExpressionMatch>
// #include <QDebug>
// #include <QTime>

// NOTE: zypper xml output has some serious limitations, the follwing code is full of hacks and workarounds and needs a huge refacoring

ZypperWrapper::ZypperWrapper(QObject *parent) : QObject(parent)
{
	processRunning = 0;
	resumeType = 0;
	autoResolveConflicts = false;
	autoAgreeLicenses = false;
	firstCheck = true;
	xmlstreamreader = new QXmlStreamReader();
    aliveTimer = new QTimer();
    connect(aliveTimer, SIGNAL(timeout()), this, SLOT(isSessionAlive()));
	aliveTimer->setInterval(60000);//1 min timer interval
	retrycheckTimer = new QTimer();
    connect(retrycheckTimer, SIGNAL(timeout()), this, SLOT(retryCheck()));
	retrycheckTimer->setInterval(60000);//1 min timer interval
// 	qDebug() << "ZypperWrapper Start " << QTime::currentTime().toString() << endl;
}

ZypperWrapper::~ZypperWrapper()
{
// 	qDebug() << "ZypperWrapper Stop " << QTime::currentTime().toString() << endl;
}

bool ZypperWrapper::isProcessRunning()
{
	if (processRunning > 0)
		return true;
	return false;
}

int ZypperWrapper::getNumpackages()
{
	return numpackages;
}

QString ZypperWrapper::getNotificationText(int type)
{
	if (type == 1) {
		return availableUpdatesText;
	} else if (type == 2) {
		return installCompletedText;
	} else if (type == 10) {
		return "Checking for updates failed: network problems";
	}
	return "";
}

void ZypperWrapper::setInstallOptions(bool autoresolveConflicts, bool autoagreeLicenses)
{
	autoResolveConflicts = autoresolveConflicts;
	autoAgreeLicenses = autoagreeLicenses;
}

void ZypperWrapper::parseXmlUpdateList()
{
	if (xmlstreamreader->name() == "install-summary") {
		if (xmlstreamreader->attributes().hasAttribute("packages-to-change")) {
			numpackages = xmlstreamreader->attributes().value("packages-to-change").toInt();
		}
	}
	if (xmlstreamreader->name() == "to-install") {
		packageTo = 1;
	} else if (xmlstreamreader->name() == "to-remove") {
		packageTo = 2;
	} else if ((xmlstreamreader->name() == "to-upgrade") || (xmlstreamreader->name() == "to-upgrade-change-arch")) {
		packageTo = 3;
	} else if ((xmlstreamreader->name() == "to-downgrade") || (xmlstreamreader->name() == "to-downgrade-change-arch")) {
		packageTo = 4;
	} else if ((xmlstreamreader->name() == "to-reinstall") || (xmlstreamreader->name() == "to-change-arch") || (xmlstreamreader->name() == "to-change-vendor")) {
		packageTo = 5;
	}
	if (xmlstreamreader->name() == "solvable") {
		int packageType = 0;
		if (xmlstreamreader->attributes().hasAttribute("type")) {
			if (xmlstreamreader->attributes().value("type") == "package") {
				packageType = 1;
			} else if (xmlstreamreader->attributes().value("type") == "product") {
				packageType = 2;
				//TODO: add message for product upgrade
			}
		}
		QString attName = "";
		QString attEdition = "";
		QString attSummary = "";
		if (xmlstreamreader->attributes().hasAttribute("name")) {
			attName = xmlstreamreader->attributes().value("name").toString();
		}
		if (xmlstreamreader->attributes().hasAttribute("edition")) {
			attEdition = xmlstreamreader->attributes().value("edition").toString();
		}
		if (xmlstreamreader->attributes().hasAttribute("summary")) {
			attSummary = xmlstreamreader->attributes().value("summary").toString();
		}
		if (packageType == 1) {
			updatesList << QVariant::fromValue(
									QVariantList{
										packageTo,
										attName,
										attEdition,
										attSummary
									}
								);
		}
	}
}

void ZypperWrapper::parseXmlInput()
{
	if (xmlstreamreader->name() == "prompt") {
		if (xmlstreamreader->attributes().hasAttribute("id")) {
			promptId = xmlstreamreader->attributes().value("id").toString();
			promptOptList.clear();
			promptOptList << promptId;
			xmlstreamreader->readNextStartElement();
			QString promptText = "";
			if ((parseStep > 0) && (promptId != "0") && !xmlmessageList.isEmpty()) {//License Agreement
				promptText = promptText + xmlmessageList.last() + "\n";
			}
			if (xmlstreamreader->name() == "description") {
				promptText = promptText + xmlstreamreader->readElementText() + "\n";
				xmlstreamreader->readNextStartElement();
			}
			if (xmlstreamreader->name() == "text") {
				promptText = promptText + xmlstreamreader->readElementText();
				xmlstreamreader->readNextStartElement();
			}
			promptOptList << promptText;
			QString attValue = "";
			QString attDesc = "";
			while (xmlstreamreader->name() == "option") {
				if (!xmlstreamreader->isEndElement()) {
					if (xmlstreamreader->attributes().hasAttribute("value")) {
						attValue = xmlstreamreader->attributes().value("value").toString();
					}
					promptOptList << attValue;
					inputValues << attValue;
					if (xmlstreamreader->attributes().hasAttribute("desc")) {
						if (xmlstreamreader->attributes().value("desc") != "") {
							attDesc = xmlstreamreader->attributes().value("desc").toString();
						} else {
							attDesc = attValue;
						}
					}
					promptOptList << attDesc;
				}
				xmlstreamreader->readNextStartElement();
			}
		}
	}
}

void ZypperWrapper::checkUpdatesOutput()
{
	aliveTimer->start();
	promptId = "";
	inputValues.clear();
	QByteArray output = wrapperProcess->readAllStandardOutput();
	xmlstream.append(output);
	if (xmlstream.trimmed().endsWith("</prompt>") || xmlstream.trimmed().endsWith("</exitcode>")) {
		xmlstreamprompt = true;
	}
	if (parsingSummary) {
		summarystream.append(output);
		searchUpdatesText();
	} else {
		xmlstreamreader->addData(output);
	}
	if (xmlstreamprompt) {
		while (!xmlstreamreader->atEnd()/* && !xmlstreamreader->hasError()*/) {
			if (xmlstreamreader->isStartElement()) {
				parseXmlUpdateList();
				if (xmlstreamreader->name() == "message") {
					if (xmlstreamreader->attributes().hasAttribute("type")) {
// 						if ((xmlstreamreader->attributes().value("type") == "error") || (xmlstreamreader->attributes().value("type") == "warning")) {
						if (xmlstreamreader->attributes().value("type") == "error") {
							QString errmess = xmlstreamreader->readElementText();
							xmlmessageErrorList << errmess;
							updatesList << QVariant::fromValue(
									QVariantList{
										100,
										"",
										"",
										errmess
									}
								);
						} else {
							xmlmessageList << xmlstreamreader->readElementText();
						}
					}
				}
				parseXmlInput();
			}
			xmlstreamreader->readNextStartElement();
		}
	}
	if (promptId != "") {
		if (promptId == "0") {
			if (parseStep > 0) {//Auto-accept
				parsingSummary = false;
				promptInputWrite("");
			} else {//Show summary
				parsingSummary = true;
				xmlstreamprompt = false;
				parseStep = 1;
				promptInputWrite(inputValues.last());
			}
		} else if (promptId != "") {
			promptInputWrite("");
			numerrorpromptcheck++;
		}
		promptOptList.clear();
	}
	if (xmlstream.trimmed().endsWith("</exitcode>")) {
		QRegularExpression reexcode("<exitcode>(.*?)<\\/exitcode>");
		QRegularExpressionMatch matchexcode = reexcode.match(xmlstream);
		xmlexitcode = matchexcode.captured(1);
		if (firstCheck && (xmlexitcode != "0")) {
			retrycheckTimer->start();
		} else if ((xmlexitcode == "106") && (numpackages == 0) && (numerrorpromptcheck == 0)) {//ZYPPER_EXIT_INF_REPOS_SKIPPED
			emit ZypperWrapper::headerMessageWrapper(getNotificationText(10));
			emit operationAbortedWrapper(10);
		} else {
			if (numpackages == 0) {
				if (xmlmessageList.size() > 0) emit headerMessageWrapper(xmlmessageList.last());
			} else {
				emit ZypperWrapper::headerMessageWrapper(availableUpdatesText);
			}
			emit ZypperWrapper::checkCompletedWrapper(false);
		}
		firstCheck = false;
		checkProcessFinished();
	}
}

void ZypperWrapper::checkUpdatesFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
	Q_UNUSED(exitCode);
	Q_UNUSED(exitStatus);
// 	qDebug() << "checkUpdatesFinished " << QTime::currentTime().toString() << " " << QString::number(exitCode) << " " << exitStatus;
	processRunning = 0;
}

void ZypperWrapper::checkUpdatesError(QProcess::ProcessError error)
{
	Q_UNUSED(error);
// 	qDebug() << "checkUpdatesError " << QTime::currentTime().toString() << " " << error;
	processRunning = 0;
}

void ZypperWrapper::checkUpdatesStart(bool checked)
{
// 	qDebug() << "checkUpdatesStart " << QTime::currentTime().toString();
	if (!checked && (QFile::exists("/tmp/twupdater-check-xml-out") || QFile::exists("/tmp/twupdater-xml-out"))) {
		if (QFile::exists("/tmp/twupdater-check-xml-out")) {
			resumeType = 1;
		} else {
			resumeType = 2;
		}
		QProcess *checkresumeProcess = new QProcess();
		connect(checkresumeProcess, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(tryResumeFinished(int, QProcess::ExitStatus)), (Qt::ConnectionType)(Qt::UniqueConnection | Qt::DirectConnection));
		connect(checkresumeProcess, SIGNAL(errorOccurred(QProcess::ProcessError)), this, SLOT(tryResumeError(QProcess::ProcessError)), (Qt::ConnectionType)(Qt::UniqueConnection | Qt::DirectConnection));
		QStringList arguments;
		checkresumeProcess->start("/bin/sh", arguments << "-c" << "screen -list | grep \"\\.twupdater-zypper-dup[[:space:]]\"");
	} else {
		resetVars();
		processRunning = 1;
		aliveTimer->start();
		wrapperProcess = new QProcess();
		wrapperProcess->setProcessChannelMode(QProcess::MergedChannels);
		connect(wrapperProcess, SIGNAL(readyRead()), this, SLOT(checkUpdatesOutput()), (Qt::ConnectionType)(Qt::UniqueConnection | Qt::DirectConnection));
		connect(wrapperProcess, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(checkUpdatesFinished(int, QProcess::ExitStatus)), (Qt::ConnectionType)(Qt::UniqueConnection | Qt::DirectConnection));
		connect(wrapperProcess, SIGNAL(errorOccurred(QProcess::ProcessError)), this, SLOT(checkUpdatesError(QProcess::ProcessError)), (Qt::ConnectionType)(Qt::UniqueConnection | Qt::DirectConnection));
		QStringList arguments;
		if (resumeType == 1 ) {
			resumeType = 0;
			wrapperProcess->start("/bin/sh", arguments << "-c" << "tail -f -n +1 /tmp/twupdater-check-xml-out");
		} else {
			wrapperProcess->start("/bin/sh", arguments << "-c" << "touch /tmp/twupdater-check-xml-out && screen -d -m -S twupdater-zypper-dup pkexec /usr/bin/zypper-dup-wrapper check && tail -f -n +1 /tmp/twupdater-check-xml-out");
		}
	}
}

void ZypperWrapper::tryResumeFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
// 	Q_UNUSED(exitCode);
	Q_UNUSED(exitStatus);
// 	qDebug() << "tryResumeFinished " << QTime::currentTime().toString() << " " << QString::number(exitCode) << " " << exitStatus;
	if (exitCode == 0) {//screen session found
		if (resumeType == 1) {
			checkUpdatesStart(true);
		} else {
			installUpdatesStart();
		}
	} else if (exitCode == 1) {//screen session not found
		if (resumeType == 1) {
			QFile::remove("/tmp/twupdater-check-xml-out");
		} else {
			QFile::remove("/tmp/twupdater-xml-out");
		}
		resumeType = 0;
		checkUpdatesStart(true);
	} else {//error
		emit operationAbortedWrapper(-1);
		resumeType = 0;
	}
}

void ZypperWrapper::tryResumeError(QProcess::ProcessError error)
{
	Q_UNUSED(error);
// 	qDebug() << "tryResumeError " << QTime::currentTime().toString() << " " << error;
	emit operationAbortedWrapper(-1);
	resumeType = 0;
}

void ZypperWrapper::retryCheck()
{
// 	qDebug() << "retryCheck " << QTime::currentTime().toString();
	retrycheckTimer->stop();
	checkUpdatesStart(false);
}

void ZypperWrapper::isSessionAlive()
{
// 	qDebug() << "isSessionAlive " << QTime::currentTime().toString();
	QProcess *checksessionProcess = new QProcess();
	connect(checksessionProcess, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(isSessionAliveFinished(int, QProcess::ExitStatus)), (Qt::ConnectionType)(Qt::UniqueConnection | Qt::DirectConnection));
	QStringList arguments;
	checksessionProcess->start("/bin/sh", arguments << "-c" << "screen -list | grep \"\\.twupdater-zypper-dup[[:space:]]\"");
}

void ZypperWrapper::isSessionAliveFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
// 	Q_UNUSED(exitCode);
	Q_UNUSED(exitStatus);
// 	qDebug() << "isSessionAliveFinished " << QTime::currentTime().toString() << " " << QString::number(exitCode) << " " << exitStatus;
	if (exitCode == 0) {//screen session found
		//TODO:add counter, if session do not send data for long time show button to resume screen session in konsole
	} else if (exitCode == 1) {//screen session not found
		if (processRunning == 1) {
			checkProcessFinished();
			emit operationAbortedWrapper(1);
		} else if (processRunning == 2) {
			installProcessFinished();
			emit operationAbortedWrapper(2);
		} else {
			aliveTimer->stop();
		}
	}
}

void ZypperWrapper::checkProcessFinished()
{
	//TODO: control PID detached process
// 	wrapperProcess->terminate();
	wrapperProcess->kill();
	aliveTimer->stop();
	QFile::remove("/tmp/twupdater-check-xml-out");
}

void ZypperWrapper::promptInputWrite(const QString inputStr)
{
// 	wrapperProcess->write(QString(inputStr + QString("\n")).toLatin1());
// 	wrapperProcess->write(QString(inputStr + QString("\n")).toUtf8());
	QProcess *writeProcess = new QProcess();
	QString sendtoscreen = "screen -S twupdater-zypper-dup -p 0 -X stuff \"" + inputStr + "^M\"";
	writeProcess->startDetached(sendtoscreen);
}

void ZypperWrapper::searchUpdatesText()
{
// 	qDebug() << "searchUpdatesText " << QTime::currentTime().toString();
	if (summarystream.trimmed().endsWith("</prompt>")) {
		QStringList summarypart = QString::fromUtf8(summarystream).split("<prompt");
		QStringList summarypart0 = summarypart.at(0).split("\n");
		QRegularExpression re("^\\d.*");
		QRegularExpressionMatch match;
		for (int i = summarypart0.count()-1; i >= 0; i--) {
			match = re.match(summarypart0.at(i));
			if (match.hasMatch()) {
				availableUpdatesText = summarypart0.at(i);
				break;
			}
		}
		xmlstreamreader->addData("<prompt" + summarypart.at(1));
	}
}

void ZypperWrapper::installUpdatesOutput()
{
// 	qDebug() << "installUpdatesOutput " << QTime::currentTime().toString();
	bool skipPrompt = false;
	aliveTimer->start();
	promptId = "";
	inputValues.clear();
	QByteArray output = wrapperProcess->readAllStandardOutput();
	xmlstream.append(output);
	if (xmlstream.trimmed().endsWith("</prompt>") || xmlstream.trimmed().endsWith("</exitcode>")) {
		xmlstreamprompt = true;
	} else if (resumeType > 0) {
		skipPrompt = true;
	}
	if (resumeType > 0) {
		if (xmlstream.contains("<download url=")) {
			parseStep = 1;
			xmlstreamprompt = true;
		}
	}
	if (parsingSummary) {
		summarystream.append(output);
		searchUpdatesText();
	} else {
		xmlstreamreader->addData(output);
	}

	if (xmlstreamprompt) {
		while (!xmlstreamreader->atEnd()/* && !xmlstreamreader->hasError()*/) {
			if (xmlstreamreader->isStartElement()) {
				parseXmlUpdateList();
				if (resumeType > 0) {
					resumeType = 0;
					emit ZypperWrapper::installResumedWrapper(numpackages);
				}
				if (xmlstreamreader->name() == "message") {
					if (xmlstreamreader->attributes().hasAttribute("type")) {
// 						if ((xmlstreamreader->attributes().value("type") == "error") || (xmlstreamreader->attributes().value("type") == "warning")) {
						if (xmlstreamreader->attributes().value("type") == "error") {
							QString errmess = xmlstreamreader->readElementText();
							xmlmessageErrorList << errmess;
							updatesList << QVariant::fromValue(
									QVariantList{
										100,
										"",
										"",
										errmess
									}
								);
						} else {
							xmlmessageList << xmlstreamreader->readElementText();
						}
					}
					//TODO: use a differnt method for message type detection
					if ((parseStep == 1) && !xmlmessageList.last().contains("\n") && xmlmessageList.last().contains("), ")) {
						QVariantList message;
						QStringList messpart = xmlmessageList.last().split("), ");
						QStringList messpart0 = messpart.at(0).split(" (");
						QStringList messpart0nl = messpart0.at(0).split(" ");
						message << "m1";
						message << messpart0nl.last();
						message << "(" + messpart0.at(1) + "), " + messpart.at(1);
						emit installMessageWrapper(message);
					} else if (parseStep >= 1) {
						xmlmessageInstallCompletedList << xmlmessageList.last();
						installCompletedText.append("<table><tr><td>" + xmlmessageList.last() + "</td></tr></table>");
					}
				}
				if ((parseStep == 1) && (xmlstreamreader->name() == "download")) {
					QString downloadPerc = "";
					QString downloadRate = "";
					QString downloadDone = "";
					if (xmlstreamreader->attributes().hasAttribute("percent")) {
						downloadPerc = xmlstreamreader->attributes().value("percent").toString();
					}
					if (xmlstreamreader->attributes().hasAttribute("rate")) {
						downloadRate = xmlstreamreader->attributes().value("rate").toString();
					}
					if (xmlstreamreader->attributes().hasAttribute("done")) {
						downloadDone = xmlstreamreader->attributes().value("done").toString();
					}
					QVariantList download;
					download << "d";
					download << downloadPerc;
					download << downloadRate;
					download << downloadDone;
					emit installMessageWrapper(download);
				}
				if ((parseStep >= 1) && (xmlstreamreader->name() == "progress")) {
					QString progressName = "";
					QString progressValue = "";
					QString progressDone = "";
					if (xmlstreamreader->attributes().hasAttribute("name")) {
						progressName = xmlstreamreader->attributes().value("name").toString();
					}
					if (xmlstreamreader->attributes().hasAttribute("value")) {
						progressValue = xmlstreamreader->attributes().value("value").toString();
					}
					if (xmlstreamreader->attributes().hasAttribute("done")) {
						progressDone = xmlstreamreader->attributes().value("done").toString();
					}
					QVariantList progress;
					progress << "p";
					progress << progressName;
					progress << progressValue;
					progress << progressDone;
					emit installMessageWrapper(progress);
					if (progressDone == "1") {
						xmlmessageInstallCompletedList.clear();
						installCompletedText.clear();
					}
				}
				if (!skipPrompt) {
					parseXmlInput();
				}
			}
			xmlstreamreader->readNextStartElement();
		}
	}
	if (promptId != "") {
		if (!parsingSummary && (promptId == "0") && (parseStep == 0)) {//start install prompt
				parsingSummary = true;
				xmlstreamprompt = false;
				parseStep = 1;
				promptInputWrite(inputValues.last());
		} else if (!parsingSummary && (promptId == "0") && (parseStep > 0)) {//notifications at the end of install process
			installCompletedText.clear();
			promptInputWrite("");//auto-accept default value (do not show)
		} else {
			if ((promptId == "10") || (promptId == "1")) xmlmessageList.clear();//clear license message and conflicts
			if ((promptId != "0") && (parseStep == 0)) {
				xmlstreamprompt = false;
			} else if (parsingSummary && (promptId == "0") && (parseStep > 0)) {
				xmlmessageErrorList.clear();
				xmlmessageList.clear();
				parsingSummary = false;
				emit ZypperWrapper::headerMessageWrapper(availableUpdatesText);
				emit ZypperWrapper::checkCompletedWrapper(true);
			}
			emit ZypperWrapper::installPromptWrapper(promptOptList);
		}
		promptOptList.clear();
	}
	if (xmlstream.trimmed().endsWith("</exitcode>")) {
		QRegularExpression reexcode("<exitcode>(.*?)<\\/exitcode>");
		QRegularExpressionMatch matchexcode = reexcode.match(xmlstream);
		xmlexitcode = matchexcode.captured(1);
		QRegularExpression re("<progress id=\"(.*)\" name=\"[(]([0-9]*)[\\/]\\2[)](.*)\" done=\"1\"[\\/]>");//match (xxx/xxx) pattern
		QRegularExpressionMatch match = re.match(xmlstream);
		if (match.hasMatch() || (xmlexitcode == "0")) {//Install completed
			QVariantList messageseparator;
			messageseparator << "m2";
			messageseparator << "\n\n\n";
			emit installMessageWrapper(messageseparator);
			QVariantList message;
			QStringListIterator messerrlistIterator(xmlmessageErrorList);
			while (messerrlistIterator.hasNext()) {//NOTE: not necessary?
				message << "m2";
				QString messagestr = messerrlistIterator.next();
				message << messagestr;
				emit installMessageWrapper(message);
				message.clear();
			}
			QStringListIterator messlistIterator(xmlmessageInstallCompletedList);
			while (messlistIterator.hasNext()) {
				message << "m2";
				QString messagestr = messlistIterator.next();
				message << messagestr;
				emit installMessageWrapper(message);
				message.clear();
			}
			emit installMessageWrapper(messageseparator);
			emit ZypperWrapper::installCompletedWrapper();
		} else {
			if (xmlmessageList.isEmpty()) {//user abort, no confirm (start or license)
				emit operationAbortedWrapper(0);
			} else {//abort install
				QVariantList message;
				QStringListIterator messerrlistIterator(xmlmessageErrorList);
				while (messerrlistIterator.hasNext()) {
					message << "m2";
					QString messagestr = messerrlistIterator.next();
					message << messagestr;
					emit installMessageWrapper(message);
					message.clear();
				}
				QStringListIterator messlistIterator(xmlmessageList);
				while (messlistIterator.hasNext()) {
					message << "m2";
					QString messagestr = messlistIterator.next();
					message << messagestr;
					emit installMessageWrapper(message);
					message.clear();
				}
				emit operationAbortedWrapper(2);
			}
		}
		installProcessFinished();
	}
}

void ZypperWrapper::installUpdatesFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
	Q_UNUSED(exitCode);
	Q_UNUSED(exitStatus);
// 	qDebug() << "installUpdatesFinished " << QTime::currentTime().toString() << " " << QString::number(exitCode) << " " << exitStatus;
	processRunning = 0;
}

void ZypperWrapper::installUpdatesError(QProcess::ProcessError error)
{
	Q_UNUSED(error);
// 	qDebug() << "installUpdatesError " << QTime::currentTime().toString() << " " << error;
	processRunning = 0;
}

void ZypperWrapper::installUpdatesStart()
{
// 	qDebug() << "installUpdatesStart " << QTime::currentTime().toString();
	resetVars();
	processRunning = 2;
	aliveTimer->start();
	wrapperProcess = new QProcess();
    connect(wrapperProcess, SIGNAL(readyRead()), this, SLOT(installUpdatesOutput()), (Qt::ConnectionType)(Qt::UniqueConnection | Qt::DirectConnection));
    connect(wrapperProcess, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(installUpdatesFinished(int, QProcess::ExitStatus)), (Qt::ConnectionType)(Qt::UniqueConnection | Qt::DirectConnection));
	connect(wrapperProcess, SIGNAL(errorOccurred(QProcess::ProcessError)), this, SLOT(installUpdatesError(QProcess::ProcessError)), (Qt::ConnectionType)(Qt::UniqueConnection | Qt::DirectConnection));
	QStringList arguments;
	if (resumeType == 2) {
// 		xmlstreamprompt = true;
		wrapperProcess->start("/bin/sh", arguments << "-c" << "tail -f -n +1 /tmp/twupdater-xml-out");
	} else {
		QString zipperparams = "";
		if (autoResolveConflicts) {
			zipperparams = zipperparams + " autoresolve";
		} else {
			zipperparams = zipperparams + " 0";
		}
		if (autoAgreeLicenses) {
			zipperparams = zipperparams + " autolicense";
		} else {
			zipperparams = zipperparams + " 0";
		}
		wrapperProcess->start("/bin/sh", arguments << "-c" << "touch /tmp/twupdater-xml-out && screen -d -m -S twupdater-zypper-dup pkexec /usr/bin/zypper-dup-wrapper install" + zipperparams + " && tail -f -n +1 /tmp/twupdater-xml-out");
	}
}

void ZypperWrapper::installProcessFinished()
{
	//TODO: control PID detached process
// 	wrapperProcess->terminate();
	wrapperProcess->kill();
	aliveTimer->stop();
	QFile::remove("/tmp/twupdater-xml-out");
}

void ZypperWrapper::resetVars()
{
	parsingSummary = false;
	availableUpdatesText.clear();
	installCompletedText.clear();
	packageTo = 0;
	numpackages = 0;
	numerrorpromptcheck = 0;
	xmlstreamprompt = false;
	parseStep = 0;
	xmlexitcode = "-1";
	xmlmessageList.clear();
	xmlmessageErrorList.clear();
	xmlmessageInstallCompletedList.clear();
	promptOptList.clear();
	xmlstream.clear();
	summarystream.clear();
	xmlstreamreader->clear();
	updatesList.clear();
}
