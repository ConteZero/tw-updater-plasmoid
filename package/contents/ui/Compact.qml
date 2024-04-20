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
import org.kde.plasma.components
// import org.kde.ksvg as KSVG
import org.kde.kirigami as Kirigami
import QtQuick.Controls

import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.plasmoid

Item {
  id: compact
  property real itemSize: Math.min(compact.height, compact.width)


  Item {
    id: container

    height: compact.itemSize
    width: height

    anchors.centerIn: parent


    Kirigami.Icon {
		id: compacticon
		source: "update-none"
		anchors.fill: parent
	}
	
	Rectangle {
		id: roundrect
		width: (numupdates > 0) ? ((numupdatestext.width * 1.3) > numupdatestext.height ? (numupdatestext.width * 1.3) : numupdatestext.height) : numupdatestext.height
		height: numupdatestext.height
		radius: Math.round(width * 0.5)
		color: "#3daee9"
		visible: (numupdates > 0) || numIndicator
		anchors {
			horizontalCenter: parent.horizontalCenter
			bottom: parent.bottom
		}
	}
	
	Text {
		id: numupdatestext
		text: numupdates
		font.pointSize: 6
		color: (numupdates > 0) ? "Black" : "transparent"
		anchors.centerIn: roundrect
		visible: roundrect.visible
	}
	
	MouseArea {
		anchors.fill: parent
		onClicked: {
			// plasmoid.expanded = !plasmoid.expanded;
			expanded = !expanded;
		}
		hoverEnabled: true
	}

  }
}
