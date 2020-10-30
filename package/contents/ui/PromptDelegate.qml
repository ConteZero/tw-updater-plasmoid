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
	id: promptDelegate

	signal checkedPromptopt(string opt)

	height: typeheader ? promptColumnHeader.height + Math.round(units.gridUnit * 0.5) : promptColumn.height + Math.round(units.gridUnit * 0.5)
// 	width: parent.width
	//width: ListView.view.width
	width: promptView.width
	enabled: true
	checked: typeheader ? false : containsMouse

	PlasmaComponents.RadioButton {
		id: radiobutton
		visible: !typeheader
		anchors {
			left: parent.left
			verticalCenter: promptColumn.verticalCenter
		}
		checked: false
		onClicked: {
// 			console.log("clicked: ", optvalue);
			checkedPromptopt(optvalue);
		}
	}
	
	Column {
		id: promptColumnHeader
		visible: typeheader
		height: promptoptLabelHeader.height
		anchors {
			left: parent.left
			right: parent.right
			top: parent.top
			leftMargin: Math.round(units.gridUnit * 0.5)
			rightMargin: Math.round(units.gridUnit * 0.5)
		}
		PlasmaComponents.Label {
			id: promptoptLabelHeader
			height: paintedHeight
			anchors {
				left: parent.left
				right: parent.right
			}
			wrapMode: Text.WordWrap
			width: parent.width
			text: promptopt
		}
	}
	
	Column {
		id: promptColumn
		visible: !typeheader
		height: promptoptLabel.height
		anchors {
			left: radiobutton.right
			right: parent.right
			top: parent.top
			leftMargin: Math.round(units.gridUnit * 0.5)
			rightMargin: Math.round(units.gridUnit * 0.5)
		}
		PlasmaComponents.Label {
			id: promptoptLabel
			height: paintedHeight
			anchors {
				left: parent.left
				right: parent.right
			}
			wrapMode: Text.WordWrap
			width: parent.width
			text: promptopt
		}
	}

}
