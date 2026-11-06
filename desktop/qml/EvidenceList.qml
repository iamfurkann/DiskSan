/** @file
 * @brief Present observed or modeled evidence as plain text, preserving unknown and blocked states.
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: evidence
    property var records: []
    property string emptyText: "No observations available. Select a device and complete an inspection."
    spacing: 12
    ReadOnlyLabel {
        Layout.fillWidth: true
        visible: evidence.records.length === 0
        text: evidence.emptyText
    }
    Repeater {
        model: evidence.records
        delegate: Frame {
            required property var modelData
            Layout.fillWidth: true
            ColumnLayout {
                anchors.fill: parent
                spacing: 4
                ReadOnlyLabel {
                    Layout.fillWidth: true
                    font.bold: true
                    text: (modelData.code || modelData.check || modelData.method || "Evidence") + " / " +
                          (modelData.severity || modelData.state || "Unknown")
                }
                ReadOnlyLabel {
                    Layout.fillWidth: true
                    text: modelData.message || modelData.explanation || modelData.detail || "No explanation reported."
                }
                ReadOnlyLabel {
                    Layout.fillWidth: true
                    opacity: 0.8
                    text: "Source: " + (modelData.source || modelData.evidence_scope || "Unknown")
                }
            }
        }
    }
}
