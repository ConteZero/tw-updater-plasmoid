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
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.extras as PlasmaExtras


Kirigami.ScrollablePage {

    id: configGeneral

	property alias cfg_checkInterval: checkInterval.value
	property alias cfg_autoagreeLicenses: autoagreeLicenses.checked
	property alias cfg_autoresolveConflicts: autoresolveConflicts.checked
	property alias cfg_enableLogging: enableLogging.checked

    ColumnLayout {

        anchors {
            left: parent.left
            top: parent.top
            right: parent.right
        }

        Kirigami.FormLayout {
            wideMode: false

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: "Check Interval"
            }
        }

        Kirigami.FormLayout {
            RowLayout {
                Kirigami.FormData.label: "Check Every (hours): "
                visible: true
                Controls.SpinBox {
					id: checkInterval
					value: 1
					from: 1
					to: 999
					Layout.alignment: Qt.AlignLeft
					onValueChanged: cfg_checkInterval = value
				}
            }
        }

        Kirigami.FormLayout {
            wideMode: false

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: "Options"
            }
        }

        Kirigami.FormLayout {
            RowLayout {
                Kirigami.FormData.label: "Auto agree with Licenses: "
                visible: true
                Controls.CheckBox {
                    id: autoagreeLicenses
                    checked: false
                }
            }

        }
        Kirigami.FormLayout {
            RowLayout {
                Kirigami.FormData.label: "Auto resolve package conflicts: "
                visible: true
                Controls.CheckBox {
                    id: autoresolveConflicts
                    checked: false
                }
            }

        }
        Kirigami.FormLayout {
            RowLayout {
                Kirigami.FormData.label: "Enable logging (on /tmp/twupdater/): "
                visible: true
                Controls.CheckBox {
                    id: enableLogging
                    checked: false
                }
            }

        }

        Kirigami.FormLayout {
            wideMode: false

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: "Info"
            }
        }
        Kirigami.FormLayout {
            RowLayout {
                Kirigami.FormData.label: "In case of problems you can run the command:"
                visible: true
            }
        }
        Kirigami.FormLayout {
            RowLayout {
                Kirigami.SelectableLabel {
                    text: "screen -r twupdater-zypper-dup"
                    font.family: "monospace"
                }
                visible: true
            }
        }
        
    }

}

