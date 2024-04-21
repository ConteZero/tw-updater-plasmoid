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
// #include <QDebug> //DEBUG
// #include <QTime> //DEBUG

// NOTE: zypper xml output has some serious limitations, the follwing code is full of hacks and workarounds and needs a huge refacoring

ZypperWrapper::ZypperWrapper(QObject *parent) : QObject(parent)
{
	processRunning = 0;
	resumeType = 0;
	autoResolveConflicts = false;
	autoAgreeLicenses = false;
	enableLogs = false;
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
		return QStringLiteral("Checking for updates failed: network problems");
	}
	return QStringLiteral("");
}

void ZypperWrapper::setInstallOptions(bool autoresolveConflicts, bool autoagreeLicenses, bool enableLogging)
{
	autoResolveConflicts = autoresolveConflicts;
	autoAgreeLicenses = autoagreeLicenses;
	enableLogs = enableLogging;
}

void ZypperWrapper::parseXmlUpdateList()
{
	if (xmlstreamreader->name() == QStringLiteral("install-summary")) {
		if (xmlstreamreader->attributes().hasAttribute("packages-to-change")) {
			numpackages = xmlstreamreader->attributes().value("packages-to-change").toInt();
		}
	}
	if (xmlstreamreader->name() == QStringLiteral("to-install")) {
		packageTo = 1;
	} else if (xmlstreamreader->name() == QStringLiteral("to-remove")) {
		packageTo = 2;
	} else if ((xmlstreamreader->name() == QStringLiteral("to-upgrade")) || (xmlstreamreader->name() == QStringLiteral("to-upgrade-change-arch"))) {
		packageTo = 3;
	} else if ((xmlstreamreader->name() == QStringLiteral("to-downgrade")) || (xmlstreamreader->name() == QStringLiteral("to-downgrade-change-arch"))) {
		packageTo = 4;
	} else if ((xmlstreamreader->name() == QStringLiteral("to-reinstall")) || (xmlstreamreader->name() == QStringLiteral("to-change-arch")) || (xmlstreamreader->name() == QStringLiteral("to-change-vendor"))) {
		packageTo = 5;
	}
	if (xmlstreamreader->name() == QStringLiteral("solvable")) {
		int packageType = 0;
		if (xmlstreamreader->attributes().hasAttribute("type")) {
			if (xmlstreamreader->attributes().value("type") == QStringLiteral("package")) {
				packageType = 1;
			} else if (xmlstreamreader->attributes().value("type") == QStringLiteral("product")) {
				packageType = 2;
				//TODO: add message for product upgrade
			}
		}
		QString attName = QStringLiteral("");
		QString attEditionOld = QStringLiteral("");
		QString attEdition = QStringLiteral("");
		QString attSummary = QStringLiteral("");
		if (xmlstreamreader->attributes().hasAttribute("name")) {
			attName = xmlstreamreader->attributes().value("name").toString();
		}
		if (xmlstreamreader->attributes().hasAttribute("edition-old")) {
			attEditionOld = xmlstreamreader->attributes().value("edition-old").toString();
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
										attEditionOld,
										attEdition,
										attSummary
									}
								);
		}
	}
}

void ZypperWrapper::parseXmlInput()
{
	if (xmlstreamreader->name() == QStringLiteral("prompt")) {
		if (xmlstreamreader->attributes().hasAttribute("id")) {
			promptId = xmlstreamreader->attributes().value("id").toString();
			promptOptList.clear();
			promptOptList << promptId;
			xmlstreamreader->readNextStartElement();
			QString promptText = QStringLiteral("");
			if ((parseStep > 0) && (promptId != QStringLiteral("0")) && !xmlmessageList.isEmpty()) {//License Agreement
				promptText = promptText + xmlmessageList.last() + QStringLiteral("\n");
			}
			if (xmlstreamreader->name() == QStringLiteral("description")) {
				promptText = promptText + xmlstreamreader->readElementText() + QStringLiteral("\n");
				xmlstreamreader->readNextStartElement();
			}
			if (xmlstreamreader->name() == QStringLiteral("text")) {
				promptText = promptText + xmlstreamreader->readElementText();
				xmlstreamreader->readNextStartElement();
			}
			promptOptList << promptText;
			QString attValue = QStringLiteral("");
			QString attDesc = QStringLiteral("");
			while (xmlstreamreader->name() == QStringLiteral("option")) {
				if (!xmlstreamreader->isEndElement()) {
					if (xmlstreamreader->attributes().hasAttribute("value")) {
						attValue = xmlstreamreader->attributes().value("value").toString();
					}
					promptOptList << attValue;
					inputValues << attValue;
					if (xmlstreamreader->attributes().hasAttribute("desc")) {
						if (xmlstreamreader->attributes().value("desc") != QStringLiteral("")) {
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
	promptId = QStringLiteral("");
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
		// qDebug() << "ZypperWrapper checkUpdatesOutput xmlstreamprompt " << QTime::currentTime().toString() << Qt::endl;
		while (!xmlstreamreader->atEnd()/* && !xmlstreamreader->hasError()*/) {
			if (xmlstreamreader->isStartElement()) {
				parseXmlUpdateList();
				if (xmlstreamreader->name() == QStringLiteral("message")) {
					if (xmlstreamreader->attributes().hasAttribute("type")) {
// 						if ((xmlstreamreader->attributes().value("type") == "error") || (xmlstreamreader->attributes().value("type") == "warning")) {
						if (xmlstreamreader->attributes().value("type") == QStringLiteral("error")) {
							QString errmess = xmlstreamreader->readElementText();
							xmlmessageErrorList << errmess;
							updatesList << QVariant::fromValue(
									QVariantList{
										100,
										QStringLiteral(""),
										QStringLiteral(""),
										QStringLiteral(""),
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
	if (promptId != QStringLiteral("")) {
		if (promptId == QStringLiteral("0")) {
			if (parseStep > 0) {//Auto-accept
				parsingSummary = false;
				promptInputWrite(QStringLiteral(""));
				// qDebug() << "ZypperWrapper promptInputWrite 1 " << QTime::currentTime().toString() << " '" << QStringLiteral("") << "'" << Qt::endl;
			} else {//Show summary
				parsingSummary = true;
				xmlstreamprompt = false;
				parseStep = 1;
				promptInputWrite(inputValues.last());
				// qDebug() << "ZypperWrapper promptInputWrite 2 " << QTime::currentTime().toString() << " '" << inputValues.last() << "'" << Qt::endl;
			}
		} else if (promptId != QStringLiteral("")) {
			promptInputWrite(QStringLiteral(""));
			// qDebug() << "ZypperWrapper promptInputWrite 3 " << QTime::currentTime().toString() << " '" << QStringLiteral("") << "'" << Qt::endl;
			numerrorpromptcheck++;
		}
		promptOptList.clear();
	}
	if (xmlstream.trimmed().endsWith("</exitcode>")) {
		// qDebug() << "ZypperWrapper checkUpdatesOutput </exitcode> " << QTime::currentTime().toString() << Qt::endl;
		QRegularExpression reexcode(QStringLiteral("<exitcode>(.*?)<\\/exitcode>"));
		// QRegularExpressionMatch matchexcode = reexcode.match(xmlstream);
		// QRegularExpressionMatch matchexcode = reexcode.match(QString::fromRawData(xmlstream.data(), xmlstream.size());
		// QRegularExpressionMatch matchexcode = reexcode.match(QString::fromUtf8(xmlstream.data(), xmlstream.size()));
		QRegularExpressionMatch matchexcode = reexcode.match(QString::fromUtf8(xmlstream));
		xmlexitcode = matchexcode.captured(1);
		if (firstCheck && (xmlexitcode != QStringLiteral("0"))) {
			retrycheckTimer->start();
		} else if ((xmlexitcode == QStringLiteral("106")) && (numpackages == 0) && (numerrorpromptcheck == 0)) {//ZYPPER_EXIT_INF_REPOS_SKIPPED
			Q_EMIT ZypperWrapper::headerMessageWrapper(getNotificationText(10));
			Q_EMIT operationAbortedWrapper(10);
		} else {
			if (numpackages == 0) {
				if (xmlmessageList.size() > 0) Q_EMIT headerMessageWrapper(xmlmessageList.last());
			} else {
				Q_EMIT ZypperWrapper::headerMessageWrapper(availableUpdatesText);
			}
			Q_EMIT ZypperWrapper::checkCompletedWrapper(false);
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
	if (!checked && (QFile::exists(QStringLiteral("/tmp/twupdater/twupdater-check-xml-out")) || QFile::exists(QStringLiteral("/tmp/twupdater/twupdater-xml-out")))) {
		if (QFile::exists(QStringLiteral("/tmp/twupdater/twupdater-check-xml-out"))) {
			resumeType = 1;
		} else {
			resumeType = 2;
		}
		QProcess *checkresumeProcess = new QProcess();
		connect(checkresumeProcess, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(tryResumeFinished(int, QProcess::ExitStatus)), (Qt::ConnectionType)(Qt::UniqueConnection | Qt::DirectConnection));
		connect(checkresumeProcess, SIGNAL(errorOccurred(QProcess::ProcessError)), this, SLOT(tryResumeError(QProcess::ProcessError)), (Qt::ConnectionType)(Qt::UniqueConnection | Qt::DirectConnection));
		QStringList arguments;
		checkresumeProcess->start(QStringLiteral("/usr/bin/sh"), arguments << QStringLiteral("-c") << QStringLiteral("screen -list | grep \"\\.twupdater-zypper-dup[[:space:]]\""));
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
			wrapperProcess->start(QStringLiteral("/usr/bin/sh"), arguments << QStringLiteral("-c") << QStringLiteral("tail -f -n +1 /tmp/twupdater/twupdater-check-xml-out"));
		} else {
			QString zipperparams = QStringLiteral("");
			if (enableLogs) {
				zipperparams = zipperparams + QStringLiteral(" logs");
			} else {
				zipperparams = zipperparams + QStringLiteral(" 0");
			}
			wrapperProcess->start(QStringLiteral("/usr/bin/sh"), arguments << QStringLiteral("-c") << QStringLiteral("mkdir -p /tmp/twupdater && touch /tmp/twupdater/twupdater-check-xml-out && screen -d -m -S twupdater-zypper-dup pkexec /usr/bin/zypper-dup-wrapper check") + zipperparams + QStringLiteral(" && tail -f -n +1 /tmp/twupdater/twupdater-check-xml-out"));
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
			QFile::remove(QStringLiteral("/tmp/twupdater/twupdater-check-xml-out"));
		} else {
			QFile::remove(QStringLiteral("/tmp/twupdater/twupdater-xml-out"));
		}
		resumeType = 0;
		checkUpdatesStart(true);
	} else {//error
		Q_EMIT operationAbortedWrapper(-1);
		resumeType = 0;
	}
}

void ZypperWrapper::tryResumeError(QProcess::ProcessError error)
{
	Q_UNUSED(error);
// 	qDebug() << "tryResumeError " << QTime::currentTime().toString() << " " << error;
	Q_EMIT operationAbortedWrapper(-1);
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
	checksessionProcess->start(QStringLiteral("/usr/bin/sh"), arguments << QStringLiteral("-c") << QStringLiteral("screen -list | grep \"\\.twupdater-zypper-dup[[:space:]]\""));
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
			Q_EMIT operationAbortedWrapper(1);
		} else if (processRunning == 2) {
			installProcessFinished();
			Q_EMIT operationAbortedWrapper(2);
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
	QFile::remove(QStringLiteral("/tmp/twupdater/twupdater-check-xml-out"));
}

void ZypperWrapper::promptInputWrite(const QString inputStr)
{
// 	wrapperProcess->write(QString(inputStr + QString("\n")).toLatin1());
// 	wrapperProcess->write(QString(inputStr + QString("\n")).toUtf8());
	// QString inputStrTmp = QStringLiteral(" ''") + inputStr + QStringLiteral("''"); //DEBUG
	// qDebug() << "ZypperWrapper promptInputWrite " << QTime::currentTime().toString() << inputStrTmp << Qt::endl;
	// qDebug() << "ZypperWrapper promptInputWrite " << QTime::currentTime().toString() << " ''" << inputStr << "''" << Qt::endl;
	QProcess *writeProcess = new QProcess();
	QStringList arguments;
	// QString sendtoscreen = QStringLiteral("screen -S twupdater-zypper-dup -p 0 -X stuff \"") + inputStr + QStringLiteral("^M\"");
	// writeProcess->startDetached(sendtoscreen);
	// writeProcess->startDetached(QStringLiteral("/usr/bin/screen"), arguments << QStringLiteral("-S") << QStringLiteral("twupdater-zypper-dup") << QStringLiteral("-p") << QStringLiteral("0") << QStringLiteral("-X") << QStringLiteral("stuff") << QStringLiteral("\"") + inputStr + QStringLiteral("^M\""));
	// QString lastArg = QStringLiteral("\"") + inputStr + QStringLiteral("^M\"");
	// writeProcess->startDetached(QStringLiteral("/usr/bin/screen"), arguments << QStringLiteral("-S") << QStringLiteral("twupdater-zypper-dup") << QStringLiteral("-p") << QStringLiteral("0") << QStringLiteral("-X") << QStringLiteral("stuff") << lastArg);
	// writeProcess->startDetached(QStringLiteral("/usr/bin/sh"), arguments << QStringLiteral("-c") << QStringLiteral("screen -S twupdater-zypper-dup -p 0 -X stuff \"^M\""));
	writeProcess->startDetached(QStringLiteral("/usr/bin/sh"), arguments << QStringLiteral("-c") << QStringLiteral("screen -S twupdater-zypper-dup -p 0 -X stuff \"") + inputStr + QStringLiteral("^M\""));
	// writeProcess->startDetached(QStringLiteral("/usr/bin/screen"), arguments << QStringLiteral("-S") << QStringLiteral("twupdater-zypper-dup") << QStringLiteral("-p") << QStringLiteral("0") << QStringLiteral("-X") << QStringLiteral("stuff") << QStringLiteral("\"^M\""));
	
 //    command = "cmd /C start \"\" cmd /K cd /d \"" + (m_editor->absPath()) + "\"";
 //    QProcess process;
 //    QStringList arguments = QProcess::splitCommand(command);
 //    if (arguments.isEmpty())
 //        return;
 //    process.setProgram(arguments.takeFirst());
 //    process.setArgument(arguments);
 //    process.startDetached();
}

void ZypperWrapper::searchUpdatesText()
{
// 	qDebug() << "searchUpdatesText " << QTime::currentTime().toString();
	if (summarystream.trimmed().endsWith("</prompt>")) {
		QStringList summarypart = QString::fromUtf8(summarystream).split(QStringLiteral("<prompt"));
		QStringList summarypart0 = summarypart.at(0).split(QStringLiteral("\n"));
		QRegularExpression re(QStringLiteral("^\\d.*"));
		QRegularExpressionMatch match;
		for (int i = summarypart0.count()-1; i >= 0; i--) {
			match = re.match(summarypart0.at(i));
			if (match.hasMatch()) {
				availableUpdatesText = summarypart0.at(i);
				break;
			}
		}
		xmlstreamreader->addData(QStringLiteral("<prompt") + summarypart.at(1));
	}
}

void ZypperWrapper::installUpdatesOutput()
{
// 	qDebug() << "installUpdatesOutput " << QTime::currentTime().toString();
	bool skipPrompt = false;
	aliveTimer->start();
	promptId = QStringLiteral("");
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
					Q_EMIT ZypperWrapper::installResumedWrapper(numpackages);
				}
				if (xmlstreamreader->name() == QStringLiteral("message")) {
					if (xmlstreamreader->attributes().hasAttribute("type")) {
// 						if ((xmlstreamreader->attributes().value("type") == "error") || (xmlstreamreader->attributes().value("type") == "warning")) {
						if (xmlstreamreader->attributes().value("type") == QStringLiteral("error")) {
							QString errmess = xmlstreamreader->readElementText();
							xmlmessageErrorList << errmess;
							updatesList << QVariant::fromValue(
									QVariantList{
										100,
										QStringLiteral(""),
										QStringLiteral(""),
										QStringLiteral(""),
										errmess
									}
								);
						} else {
							xmlmessageList << xmlstreamreader->readElementText();
						}
					}
					//TODO: use a differnt method for message type detection
					if ((parseStep == 1) && !xmlmessageList.last().contains(QStringLiteral("\n")) && xmlmessageList.last().contains(QStringLiteral("), "))) {
						QVariantList message;
						QStringList messpart = xmlmessageList.last().split(QStringLiteral("), "));
						QStringList messpart0 = messpart.at(0).split(QStringLiteral(" ("));
						QStringList messpart0nl = messpart0.at(0).split(QStringLiteral(" "));
						QString messtmp = QStringLiteral("(") + messpart0.at(1) + QStringLiteral("), ") + messpart.at(1);
						message << QStringLiteral("m1");
						message << messpart0nl.last();
						// message << QStringLiteral("(") + messpart0.at(1) + QStringLiteral("), ") + messpart.at(1);
						message << messtmp;
						Q_EMIT installMessageWrapper(message);
					} else if (parseStep >= 1) {
						xmlmessageInstallCompletedList << xmlmessageList.last();
						installCompletedText.append(QStringLiteral("<table><tr><td>") + xmlmessageList.last() + QStringLiteral("</td></tr></table>"));
					}
				}
				if ((parseStep == 1) && (xmlstreamreader->name() == QStringLiteral("download"))) {
					QString downloadPerc = QStringLiteral("");
					QString downloadRate = QStringLiteral("");
					QString downloadDone = QStringLiteral("");
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
					download << QStringLiteral("d");
					download << downloadPerc;
					download << downloadRate;
					download << downloadDone;
					Q_EMIT installMessageWrapper(download);
				}
				if ((parseStep >= 1) && (xmlstreamreader->name() == QStringLiteral("progress"))) {
					QString progressName = QStringLiteral("");
					QString progressValue = QStringLiteral("");
					QString progressDone = QStringLiteral("");
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
					progress << QStringLiteral("p");
					progress << progressName;
					progress << progressValue;
					progress << progressDone;
					Q_EMIT installMessageWrapper(progress);
					if (progressDone == QStringLiteral("1")) {
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
	if (promptId != QStringLiteral("")) {
		if (!parsingSummary && (promptId == QStringLiteral("0")) && (parseStep == 0)) {//start install prompt
				parsingSummary = true;
				xmlstreamprompt = false;
				parseStep = 1;
				promptInputWrite(inputValues.last());
		} else if (!parsingSummary && (promptId == QStringLiteral("0")) && (parseStep > 0)) {//notifications at the end of install process
			installCompletedText.clear();
			promptInputWrite(QStringLiteral(""));//auto-accept default value (do not show)
		} else {
			if ((promptId == QStringLiteral("10")) || (promptId == QStringLiteral("1"))) xmlmessageList.clear();//clear license message and conflicts
			if ((promptId != QStringLiteral("0")) && (parseStep == 0)) {
				xmlstreamprompt = false;
			} else if (parsingSummary && (promptId == QStringLiteral("0")) && (parseStep > 0)) {
				xmlmessageErrorList.clear();
				xmlmessageList.clear();
				parsingSummary = false;
				Q_EMIT ZypperWrapper::headerMessageWrapper(availableUpdatesText);
				Q_EMIT ZypperWrapper::checkCompletedWrapper(true);
			}
			Q_EMIT ZypperWrapper::installPromptWrapper(promptOptList);
		}
		promptOptList.clear();
	}
	if (xmlstream.trimmed().endsWith("</exitcode>")) {
		QRegularExpression reexcode(QStringLiteral("<exitcode>(.*?)<\\/exitcode>"));
		// QRegularExpressionMatch matchexcode = reexcode.match(xmlstream);
		// QRegularExpressionMatch matchexcode = reexcode.match(QString::fromUtf8(xmlstream.data(), xmlstream.size()));
		QRegularExpressionMatch matchexcode = reexcode.match(QString::fromUtf8(xmlstream));
		xmlexitcode = matchexcode.captured(1);
		QRegularExpression re(QStringLiteral("<progress id=\"(.*)\" name=\"[(]([0-9]*)[\\/]\\2[)](.*)\" done=\"1\"[\\/]>"));//match (xxx/xxx) pattern
		// QRegularExpressionMatch match = re.match(xmlstream);
		// QRegularExpressionMatch match = re.match(QString::fromUtf8(xmlstream.data(), xmlstream.size()));
		QRegularExpressionMatch match = re.match(QString::fromUtf8(xmlstream));
		if (match.hasMatch() || (xmlexitcode == QStringLiteral("0"))) {//Install completed
			QVariantList messageseparator;
			messageseparator << QStringLiteral("m2");
			messageseparator << QStringLiteral("\n\n\n");
			Q_EMIT installMessageWrapper(messageseparator);
			QVariantList message;
			QStringListIterator messerrlistIterator(xmlmessageErrorList);
			while (messerrlistIterator.hasNext()) {//NOTE: not necessary?
				message << QStringLiteral("m2");
				QString messagestr = messerrlistIterator.next();
				message << messagestr;
				Q_EMIT installMessageWrapper(message);
				message.clear();
			}
			QStringListIterator messlistIterator(xmlmessageInstallCompletedList);
			while (messlistIterator.hasNext()) {
				message << QStringLiteral("m2");
				QString messagestr = messlistIterator.next();
				message << messagestr;
				Q_EMIT installMessageWrapper(message);
				message.clear();
			}
			Q_EMIT installMessageWrapper(messageseparator);
			Q_EMIT ZypperWrapper::installCompletedWrapper();
		} else {
			if (xmlmessageList.isEmpty()) {//user abort, no confirm (start or license)
				Q_EMIT operationAbortedWrapper(0);
			} else {//abort install
				QVariantList message;
				QStringListIterator messerrlistIterator(xmlmessageErrorList);
				while (messerrlistIterator.hasNext()) {
					message << QStringLiteral("m2");
					QString messagestr = messerrlistIterator.next();
					message << messagestr;
					Q_EMIT installMessageWrapper(message);
					message.clear();
				}
				QStringListIterator messlistIterator(xmlmessageList);
				while (messlistIterator.hasNext()) {
					message << QStringLiteral("m2");
					QString messagestr = messlistIterator.next();
					message << messagestr;
					Q_EMIT installMessageWrapper(message);
					message.clear();
				}
				Q_EMIT operationAbortedWrapper(2);
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
		wrapperProcess->start(QStringLiteral("/usr/bin/sh"), arguments << QStringLiteral("-c") << QStringLiteral("tail -f -n +1 /tmp/twupdater/twupdater-xml-out"));
	} else {
		QString zipperparams = QStringLiteral("");
		if (autoResolveConflicts) {
			zipperparams = zipperparams + QStringLiteral(" autoresolve");
		} else {
			zipperparams = zipperparams + QStringLiteral(" 0");
		}
		if (autoAgreeLicenses) {
			zipperparams = zipperparams + QStringLiteral(" autolicense");
		} else {
			zipperparams = zipperparams + QStringLiteral(" 0");
		}
		if (enableLogs) {
			zipperparams = zipperparams + QStringLiteral(" logs");
		} else {
			zipperparams = zipperparams + QStringLiteral(" 0");
		}
		wrapperProcess->start(QStringLiteral("/usr/bin/sh"), arguments << QStringLiteral("-c") << QStringLiteral("mkdir -p /tmp/twupdater && touch /tmp/twupdater/twupdater-xml-out && screen -d -m -S twupdater-zypper-dup pkexec /usr/bin/zypper-dup-wrapper install") + zipperparams + QStringLiteral(" && tail -f -n +1 /tmp/twupdater/twupdater-xml-out"));
	}
}

void ZypperWrapper::installProcessFinished()
{
	//TODO: control PID detached process
// 	wrapperProcess->terminate();
	wrapperProcess->kill();
	aliveTimer->stop();
	QFile::remove(QStringLiteral("/tmp/twupdater/twupdater-xml-out"));
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
	xmlexitcode = QStringLiteral("-1");
	xmlmessageList.clear();
	xmlmessageErrorList.clear();
	xmlmessageInstallCompletedList.clear();
	promptOptList.clear();
	xmlstream.clear();
	summarystream.clear();
	xmlstreamreader->clear();
	updatesList.clear();
}
