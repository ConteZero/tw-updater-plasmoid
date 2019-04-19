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
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.private.twupdater 1.0

PlasmaComponents.ListItem {
	id: checkDelegate

	height: checkColumn.height + Math.round(units.gridUnit * 0.5)
	width: parent.width
	enabled: true
	checked: containsMouse

	PlasmaCore.IconItem {
		id: updateicontype
		anchors {
			left: parent.left
			verticalCenter: checkColumn.verticalCenter
		}
		source: getIconType(type)
		height: checkColumn.height
	}

	Column {
		id: checkColumn
		height: nameVersionLabel.height + summaryLabel.height
		anchors {
			left: updateicontype.right
			right: parent.right
			top: parent.top
			leftMargin: Math.round(units.gridUnit * 0.5)
			rightMargin: Math.round(units.gridUnit * 0.5)
		}
		PlasmaComponents.Label {
			id: nameVersionLabel
			height: paintedHeight
			visible: ((name == "") && (version == "")) ? false : true
			anchors {
				left: parent.left
				right: parent.right
			}
			elide: Text.ElideRight;
			text: name + "  (" + version + ")"
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
			font.pointSize: theme.smallestFont.pointSize;
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
