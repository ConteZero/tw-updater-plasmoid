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
	id: promptDelegate

	signal checkedPromptopt(string opt)

	height: typeheader ? promptColumnHeader.height + Math.round(Kirigami.Units.gridUnit * 0.5) : promptColumn.height + Math.round(Kirigami.Units.gridUnit * 0.5)
	width: promptView.width
	enabled: !typeheader
	hoverEnabled: !typeheader
	// checked: typeheader ? false : containsMouse

	Rectangle {
        id: separator
        width: parent.width
        height: 1
        color: Kirigami.Theme.textColor
        opacity: 0.25
        visible: typeheader
    }
	PlasmaComponents.RadioButton {
		id: radiobutton
		visible: !typeheader
		anchors {
			left: parent.left
			verticalCenter: promptColumn.verticalCenter
		}
		checked: false
		onClicked: {
// 			console.log("clicked: ", optvalue)
			checkedPromptopt(optvalue)
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
			topMargin: Math.round(Kirigami.Units.gridUnit * 0.3)
			leftMargin: Math.round(Kirigami.Units.gridUnit * 0.5)
			rightMargin: Math.round(Kirigami.Units.gridUnit * 0.5)
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
			leftMargin: Math.round(Kirigami.Units.gridUnit * 0.5)
			rightMargin: Math.round(Kirigami.Units.gridUnit * 0.5)
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
