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
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.private.twupdater 1.0

PlasmaComponents.ListItem {
    id: installDelegate

	height: installColumn.contentHeight
	width: parent.width
	enabled: true
    checked: infomessage ? false : containsMouse

	Column {
		id: installColumn
		height: installTextLabel.height + installTextDetailsLabel.height + installProgressBar.height
//		height: installTextLabel.contentHeight + installTextDetailsLabel.contentHeight + installProgressBar.contentHeight
		anchors {
			left: parent.left
			right: parent.right
			top: parent.top
			leftMargin: Math.round(units.gridUnit * 0.5)
			rightMargin: Math.round(units.gridUnit * 0.5)
		}
		PlasmaComponents3.Label {
			id: installTextLabel
			height: contentHeight
			anchors {
				left: parent.left
				right: parent.right
			}
			wrapMode: Text.WordWrap
// 			width: parent.width
			text: installtext
		}
		PlasmaComponents3.Label {
			id: installTextDetailsLabel
			height: detailsvis ? paintedHeight : 0
			anchors {
				left: parent.left
				right: parent.right
			}
			visible: detailsvis
			elide: Text.ElideRight;
			font.pointSize: theme.smallestFont.pointSize;
			opacity: 0.6;
			text: details
		}
		PlasmaComponents3.ProgressBar {
			id: installProgressBar
			height: progressvis ? installProgressBar.paintedHeight : 0
			anchors {
				left: parent.left
				right: parent.right
			}
			visible: progressvis
// 			minimumValue: 0
// 			maximumValue: 100
			from: 0
			to: 100
			value: percentage
		}
	}

}
