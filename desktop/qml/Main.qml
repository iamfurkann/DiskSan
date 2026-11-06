/** @file
 * @brief Present live read-only inventory, observed evidence and explicitly non-executable simulation previews.
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import DiskSan.Backend 1.0

ApplicationWindow {
    id: window
    objectName: "diskSanWindow"
    required property DeviceService service
    required property string versionText
    property bool closingReady: false
    property int page: 0
    property var navigationItems: []
    readonly property bool hasSelection: Object.keys(service.selected).length > 0
    readonly property var observedDevice: service.information.devices && service.information.devices.length > 0 ?
                                         service.information.devices[0] : service.selected
    readonly property var pageFocus: page === 0 && deviceList.count > 0 ? deviceList.currentItem :
                                     page === 2 ? planAction : refreshAction
    width: 1140
    height: 800
    minimumWidth: 820
    minimumHeight: 620
    visible: true
    font.pixelSize: 14
    title: "DiskSan " + versionText + (service.demo ? " / DEMO DATA" : " / Read-only inspection")
    Component.onCompleted: refreshAction.forceActiveFocus(Qt.TabFocusReason)

    /** @brief Preserve missing and boolean values without rounding exact decimal capacities. */
    function display(value) {
        if (value === undefined || value === null || value === "") return "Unknown"
        if (value === true) return "Yes"
        if (value === false) return "No"
        return String(value).replace(/[\u0000-\u0008\u000b\u000c\u000e-\u001f\u007f]/g, "\ufffd").slice(0, 16384)
    }
    onClosing: function(close) {
        if (!closingReady) { close.accepted = false; service.shutdown() }
    }
    Shortcut { sequences: [StandardKey.Refresh]; onActivated: service.refresh() }
    Shortcut { sequences: [StandardKey.Quit]; onActivated: service.shutdown() }
    header: ToolBar {
        implicitHeight: 60
        RowLayout {
            anchors.fill: parent
            anchors.margins: 12
            ReadOnlyLabel { text: "DiskSan"; font.pixelSize: 24; font.bold: true }
            ReadOnlyLabel { text: window.versionText + " / Desktop alpha" }
            Item { Layout.fillWidth: true }
            ReadOnlyLabel { text: "READ-ONLY INSPECTION"; font.bold: true }
        }
    }
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12
        Frame {
            Layout.fillWidth: true
            visible: service.demo
            ReadOnlyLabel {
                objectName: "demoBanner"
                anchors.fill: parent
                text: "DEMO DATA - SYNTHETIC FIXTURES. These records are not a live device inventory."
                font.bold: true
            }
        }
        Frame {
            Layout.fillWidth: true
            RowLayout {
                anchors.fill: parent
                ColumnLayout {
                    Layout.fillWidth: true
                    ReadOnlyLabel { objectName: "statusState"; text: service.state; font.bold: true }
                    ReadOnlyLabel { objectName: "statusMessage"; Layout.fillWidth: true; text: service.message }
                }
                Button {
                    id: refreshAction
                    objectName: "refreshButton"
                    activeFocusOnTab: true
                    focus: true
                    KeyNavigation.tab: cancelAction.visible ? cancelAction : window.navigationItems[0] || null
                    text: "Refresh"; Accessible.name: "Refresh read-only device inventory"
                    enabled: service.state !== "Closing"
                    onClicked: service.refresh()
                }
                Button {
                    id: cancelAction
                    objectName: "cancelButton"
                    activeFocusOnTab: true
                    KeyNavigation.tab: window.navigationItems[0] || null
                    text: "Cancel inspection"; visible: service.busy; enabled: service.state !== "Closing"
                    Accessible.name: text
                    onClicked: service.cancel()
                }
            }
        }
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 16
            ColumnLayout {
                Layout.preferredWidth: 180
                Layout.fillHeight: true
                spacing: 8
                Repeater {
                    id: navigation
                    model: ["Devices", "Assessment", "Simulation", "About && limitations"]
                    // itemAt() does not notify bindings when later delegates become available.
                    onItemAdded: function(index, item) {
                        const items = window.navigationItems.slice()
                        items[index] = item
                        window.navigationItems = items
                    }
                    delegate: Button {
                        required property int index
                        required property string modelData
                        objectName: "navigation" + index
                        activeFocusOnTab: true
                        KeyNavigation.priority: KeyNavigation.BeforeItem
                        KeyNavigation.tab: index === 0 && !window.hasSelection ? window.navigationItems[3] || null :
                                           index < 3 ? window.navigationItems[index + 1] || null : window.pageFocus
                        Layout.fillWidth: true
                        text: modelData
                        Accessible.name: modelData.replace(/&&/g, "&")
                        highlighted: window.page === index
                        enabled: index === 0 || index === 3 || window.hasSelection
                        onClicked: window.page = index
                    }
                }
                Item { Layout.fillHeight: true }
                ReadOnlyLabel {
                    Layout.fillWidth: true
                    text: "Snapshots are non-atomic. No observation, plan or simulation authorizes erasure."
                }
            }
            StackLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: window.page
                ColumnLayout {
                    spacing: 12
                    ReadOnlyLabel { text: "Observed devices"; font.pixelSize: 22; font.bold: true }
                    ReadOnlyLabel {
                        Layout.fillWidth: true
                        text: service.devices.length + " records / Includes partitions, volumes and snapshots."
                    }
                    ReadOnlyLabel {
                        Layout.fillWidth: true
                        visible: service.devices.length === 0
                        text: service.state === "Empty" ? "No devices reported. This is not an all-clear result." :
                              "Refresh to request a read-only inventory from the configured DiskSan CLI."
                    }
                    ListView {
                        id: deviceList
                        objectName: "deviceList"
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        model: service.devices
                        keyNavigationEnabled: true
                        onCurrentItemChanged: if (activeFocus && currentItem) currentItem.forceActiveFocus(Qt.OtherFocusReason)
                        clip: true
                        spacing: 8
                        ScrollBar.vertical: ScrollBar {}
                        delegate: ItemDelegate {
                            required property var modelData
                            required property int index
                            width: deviceList.width
                            objectName: "deviceRow" + index
                            activeFocusOnTab: true
                            KeyNavigation.tab: viewAssessment.visible ? viewAssessment : refreshAction
                            KeyNavigation.backtab: window.navigationItems[3] || null
                            Keys.forwardTo: [deviceList]
                            onActiveFocusChanged: if (activeFocus) deviceList.currentIndex = index
                            highlighted: window.hasSelection && service.selected.path === modelData.path
                            Accessible.name: window.display(modelData.path) + ", " + window.display(modelData.device_role)
                            onClicked: service.selectDevice(index)
                            contentItem: ColumnLayout {
                                spacing: 4
                                ReadOnlyLabel { Layout.fillWidth: true; text: window.display(modelData.model); font.bold: true }
                                ReadOnlyLabel { Layout.fillWidth: true; text: window.display(modelData.path) }
                                ReadOnlyLabel {
                                    Layout.fillWidth: true
                                    text: window.display(modelData.platform) + " / " + window.display(modelData.device_role) +
                                          " / " + window.display(modelData.size_bytes) + " bytes"
                                }
                                ReadOnlyLabel { Layout.fillWidth: true; text: "Usage and physical scope: not established by inventory" }
                            }
                        }
                    }
                    Frame {
                        Layout.fillWidth: true
                        visible: window.hasSelection
                        ColumnLayout {
                            anchors.fill: parent
                            ReadOnlyLabel { Layout.fillWidth: true; text: "Selected: " + window.display(service.selected.path); font.bold: true }
                            ReadOnlyLabel {
                                Layout.fillWidth: true
                                text: service.fresh ? "Current response received. Open Assessment or Simulation to review its limits." :
                                                    "No current selected response. Inspection may be pending, failed or canceled."
                            }
                            Button {
                                id: viewAssessment
                                text: "View assessment"; activeFocusOnTab: true; Accessible.name: text
                                KeyNavigation.tab: refreshAction
                                enabled: service.fresh; onClicked: window.page = 1
                            }
                        }
                    }
                }
                ScrollView {
                    clip: true
                    contentWidth: availableWidth
                    ColumnLayout {
                        width: parent.width
                        spacing: 12
                        ReadOnlyLabel { text: "Assessment and evidence"; font.pixelSize: 22; font.bold: true }
                        TextArea {
                            objectName: "selectedPath"
                            Layout.fillWidth: true
                            text: window.display(service.selected.path)
                            readOnly: true; selectByMouse: true; textFormat: TextEdit.PlainText
                            wrapMode: TextEdit.Wrap; background: null; padding: 0
                            Accessible.name: "Selected device path: " + text
                        }
                        ReadOnlyLabel {
                            Layout.fillWidth: true
                            text: service.information.devices ? "Device metadata from the selected info response" :
                                                               "Inventory metadata / No current info response"
                            font.bold: true
                        }
                        Repeater {
                            model: ["model", "vendor", "serial", "device_id", "device_role", "size_bytes", "logical_sector_size",
                                    "physical_sector_size", "read_only", "transport", "protocol", "parent", "controller"]
                            delegate: TextArea {
                                required property string modelData
                                Layout.fillWidth: true
                                readOnly: true; selectByMouse: true; textFormat: TextEdit.PlainText
                                wrapMode: TextEdit.Wrap; background: null; padding: 0
                                Accessible.name: text
                                text: modelData + ": " + window.display(window.observedDevice[modelData])
                            }
                        }
                        ReadOnlyLabel {
                            Layout.fillWidth: true
                            text: "Identity applies only to its observation window. It is non-atomic and provides no future guarantee."
                        }
                        ReadOnlyLabel {
                            objectName: "assessmentStatus"
                            Layout.fillWidth: true
                            font.bold: true
                            text: "Inspection status: " + window.display(service.assessment.inspection_status)
                        }
                        ReadOnlyLabel { text: "Observed policy findings"; font.bold: true }
                        EvidenceList { Layout.fillWidth: true; records: service.assessment.findings || [] }
                        ReadOnlyLabel { text: "Original observations"; font.bold: true }
                        EvidenceList { Layout.fillWidth: true; records: service.assessment.observations || [] }
                        ReadOnlyLabel { text: "Independent capability evidence / Metadata only"; font.bold: true }
                        ReadOnlyLabel {
                            Layout.fillWidth: true
                            text: "Support does not establish access, applicability, passthrough or authorization. Desktop never requests --probe."
                        }
                        Repeater {
                            model: service.capabilities.capabilities || []
                            delegate: Frame {
                                required property var modelData
                                Layout.fillWidth: true
                                ColumnLayout {
                                    anchors.fill: parent
                                    ReadOnlyLabel { Layout.fillWidth: true; text: window.display(modelData.method); font.bold: true }
                                    ReadOnlyLabel {
                                        Layout.fillWidth: true
                                        text: "Support: " + window.display(modelData.support) + "\nOS access: " + window.display(modelData.os_access) +
                                              "\nApplicability: " + window.display(modelData.applicability) + "\nPassthrough: " + window.display(modelData.passthrough)
                                    }
                                    EvidenceList { Layout.fillWidth: true; records: modelData.evidence || [] }
                                }
                            }
                        }
                    }
                }
                ColumnLayout {
                    spacing: 12
                    ReadOnlyLabel { text: "Inspection model preview"; font.pixelSize: 22; font.bold: true }
                    Frame {
                        Layout.fillWidth: true
                        ReadOnlyLabel {
                            objectName: "simulationBanner"
                            anchors.fill: parent
                            font.bold: true
                            text: "SIMULATION ONLY - NO STORAGE OPERATION WILL RUN\nDOES NOT ERASE DATA"
                        }
                    }
                    RowLayout {
                        Button {
                            id: planAction
                            objectName: "planButton"
                            activeFocusOnTab: true
                            KeyNavigation.tab: simulationAction
                            KeyNavigation.backtab: window.navigationItems[3] || null
                            text: "Generate inspection plan"
                            Accessible.name: text
                            enabled: service.fresh && !service.busy
                            onClicked: service.generatePlan()
                        }
                        Button {
                            id: simulationAction
                            objectName: "simulationButton"
                            activeFocusOnTab: true
                            KeyNavigation.tab: refreshAction
                            text: "Preview simulated steps"
                            Accessible.name: text
                            enabled: service.fresh && !service.busy
                            onClicked: service.previewSimulation()
                        }
                    }
                    ReadOnlyLabel {
                        Layout.fillWidth: true
                        text: "Execution unavailable. Erase authorized: NO. Verification unavailable. Each request re-observes the target and replaces earlier results."
                    }
                    ScrollView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        contentWidth: availableWidth
                        ColumnLayout {
                            width: parent.width
                            ReadOnlyLabel {
                                Layout.fillWidth: true
                                font.bold: true
                                text: "Inspection status: " + window.display(service.simulation.inspection_status || service.plan.inspection_status)
                            }
                            EvidenceList {
                                Layout.fillWidth: true
                                records: service.simulation.findings || service.plan.findings || []
                            }
                            ReadOnlyLabel { text: "Hypothetical prerequisites"; font.bold: true }
                            EvidenceList {
                                Layout.fillWidth: true
                                records: service.simulation.prerequisites || service.plan.prerequisites || []
                            }
                            ReadOnlyLabel { text: "Modeled steps / No operation attempted"; font.bold: true }
                            EvidenceList { Layout.fillWidth: true; records: service.simulation.steps || [] }
                            ReadOnlyLabel {
                                objectName: "simulationResult"
                                Layout.fillWidth: true
                                text: window.display(service.simulation.simulation_result)
                            }
                        }
                    }
                }
                ScrollView {
                    clip: true
                    contentWidth: availableWidth
                    ColumnLayout {
                        width: parent.width
                        spacing: 16
                        ReadOnlyLabel { text: "About & limitations"; font.pixelSize: 22; font.bold: true }
                        ReadOnlyLabel {
                            Layout.fillWidth: true
                            text: "DiskSan " + window.versionText + " is an Apache-2.0 storage-discovery and read-only assessment application. This optional desktop is an alpha prototype."
                        }
                        ReadOnlyLabel {
                            Layout.fillWidth: true
                            text: "This version cannot erase, sanitize, format or discard disk data. A simulation is a non-executable model, not an operation, authorization or certificate."
                        }
                        ReadOnlyLabel {
                            Layout.fillWidth: true
                            text: "Observations cannot establish global exclusive access, complete host-wide usage or physical controller scope. Unknown and unavailable evidence remain unresolved."
                        }
                        ReadOnlyLabel {
                            Layout.fillWidth: true
                            text: "The standalone CLI supports an explicit audited Linux NVMe identification query. Desktop does not initiate it, request elevation or accept arbitrary commands."
                        }
                        ReadOnlyLabel {
                            Layout.fillWidth: true
                            text: "The Qt desktop has separate deployment, operating-system and license requirements from the C CLI. Consult the repository's desktop build and acceptance documentation for tested platforms."
                        }
                    }
                }
            }
        }
    }
}
