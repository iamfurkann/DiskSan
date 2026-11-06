/** @file
 * @brief Display bounded untrusted metadata strictly as plain text with accessible wrapping.
 */
import QtQuick
import QtQuick.Controls

Label {
    textFormat: Text.PlainText
    wrapMode: Text.Wrap
    Accessible.role: Accessible.StaticText
    Accessible.name: text
}
