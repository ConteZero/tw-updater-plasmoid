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

import QtQuick 2.2
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.5
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.plasma.private.twupdater 1.0

Item {
	id: genconfig
    
	property alias cfg_checkInterval: checkInterval.value
	property alias cfg_autoagreeLicenses: licenseCheckBox.checked
	property alias cfg_autoresolveConflicts: conflictCheckBox.checked

	GridLayout {
		id: optionsGroup
		anchors.left: parent.left
		anchors.right: parent.right
		Layout.fillWidth: true
		rowSpacing: 20
		columnSpacing: 15
		columns: 2
		Label {
			text: "Check Every (hours)"
		}
		SpinBox {
			id: checkInterval
			value: 1
			from: 1
			to: 999
			Layout.alignment: Qt.AlignLeft
			onValueChanged: cfg_checkInterval = value
		}
		Label {
			text: "Auto agree with Licenses"
		}
		CheckBox {
			id: licenseCheckBox
			Layout.fillWidth: true
			Layout.alignment: Qt.AlignLeft
		}
		Label {
			text: "Auto resolve package conflicts"
		}
		CheckBox {
			id: conflictCheckBox
			Layout.fillWidth: true
			Layout.alignment: Qt.AlignLeft
		}
	}
}
