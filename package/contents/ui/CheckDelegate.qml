/*
 *   Copyright (C) 2019-2024 by Michele Cherici <contact@contezero.com> 
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

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PlasmaComponents
import org.kde.plasma.extras as PlasmaExtras
import org.kde.plasma.core as PlasmaCore


PlasmaComponents.ItemDelegate {
	id: checkDelegate

	height: checkColumn.height + Math.round(Kirigami.Units.gridUnit * 0.5)
// 	width: parent.width
	//width: ListView.view.width
	width: checkView.width
	enabled: true
	// checked: containsMouse

	// PlasmaCore.IconItem {
	Kirigami.Icon {
		id: updateicontype
		anchors {
			left: parent.left
			top: parent.top
			// verticalCenter: checkColumn.verticalCenter
		}
		source: getIconType(type)
		// height: checkColumn.height*0.8
		// width: Kirigami.Theme.defaultFont.pointSize*3
		width: Kirigami.Units.gridUnit*1.5
	}

	Column {
		id: checkColumn
		height: nameVersionLabel.height + summaryLabel.height
		anchors {
			left: updateicontype.right
			right: parent.right
			top: parent.top
			leftMargin: Math.round(Kirigami.Units.gridUnit * 0.5)
			rightMargin: Math.round(Kirigami.Units.gridUnit * 0.5)
		}
		PlasmaComponents.Label {
			id: nameVersionLabel
			// height: paintedHeight
			height: contentHeight
			visible: ((name == "") && (version == "")) ? false : true
			anchors {
				left: parent.left
				right: parent.right
			}
			// elide: Text.ElideRight;
			wrapMode: Text.WordWrap
			// text: name + "  (" + versionold + " » " + version +  ")"
			text: name + "  (<font color=\"" + (hovered ? Kirigami.Theme.negativeTextColor : Kirigami.Theme.textColor) + "\">" + versionold + "</font> » <font color=\"" + (hovered ? Kirigami.Theme.positiveTextColor : Kirigami.Theme.textColor) + "\">" + version +  "</font>)"
		}
		PlasmaComponents.Label {
			id: summaryLabel
// 			height: paintedHeight
			height: contentHeight
			anchors {
				left: parent.left
				right: parent.right
			}
// 			elide: Text.ElideRight;
			wrapMode: Text.WordWrap
			font.pointSize: Kirigami.Theme.smallFont.pointSize
			opacity: nameVersionLabel.visible ? 0.6 : 1
			text: summary
		}
	}

	function getIconType(typeid) {
		if (typeid == "1") {
			return "package-install-auto";
		} else if (typeid == "2") {
			return "package-remove-auto";
		} else if (typeid == "3") {
			return "package-upgrade-auto";
		} else if (typeid == "4") {
			return "package-downgrade";
		} else if (typeid == "5") {
			return "package-reinstall";
		} else if (typeid == "100") {
			return "dialog-warning";
		}
		return "package-upgrade-auto"
	}

}
