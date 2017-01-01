#ifndef SETTINGDIALOG_H
#define SETTINGDIALOG_H

#include <QDialog>
#include <QSettings>
#include <QAbstractButton>

#include <cstdint>

enum class SettingGeneralKey { Height = 0, Position = 1, Transparency = 2, ShowCharge = 3, Align = 4, MainMonitor = 5, CustomMonitor = 6 };
enum class SettingBasicColorKey { DefaultColor = 0, ChargeColor = 1, FullColor = 2 };
enum class SettingCustomColorKey { Enable = 0, LowEdge = 1, HighEdge = 2, Color = 3 };
enum class SettingMonitor { Primary = 0, Custom = 1 };
enum class SettingPosition { Top = 0, Bottom = 1, Left = 2, Right = 3 };
enum class SettingAlign { LeftTop = 0, RightBottom = 1 };

#define BL_COLOR_LEVEL      8
#define BL_DEFAULT_DISABLED_COLOR   QColor(255, 255, 255)
struct bl_option
{
    uint32_t height;		// Battery line's height (in pixel)
    uint32_t position;		// Where to show battery line? (TOP | BOTTOM | LEFT | RIGHT)
    uint32_t transparency;	// Transparency of battery line
    bool showCharge;		// Show battery line when charging?
    uint32_t align;         // Align to Left/Top or Right/Bottom?
    bool mainMonitor;		// Which monitor to show battery line?
    uint32_t customMonitor;	// Which monitor to show battery line?
    QColor defaultColor;	// Battery line's default color
    QColor chargeColor;	    // Battery line's color when charging
    QColor fullColor;		// Battery line's color when charging is done
    bool customEnable[BL_COLOR_LEVEL];  // User defined battery line's color count
    uint32_t lowEdge[BL_COLOR_LEVEL];       // User defined edges to pick up user defined color
    uint32_t highEdge[BL_COLOR_LEVEL];      // User defined edges to pick up user defined color
    QColor customColor[BL_COLOR_LEVEL];     // User defined battery line's color
};
typedef struct bl_option BL_OPTION;

namespace Ui {
    class SettingDialog;
}

class SettingDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingDialog(BL_OPTION option, BL_OPTION defaultOption, QWidget *parent = 0);
    ~SettingDialog();

signals:
    void SignalGeneral(SettingGeneralKey key, QVariant entry);
    void SignalBasicColor(SettingBasicColorKey key, QVariant entry);
    void SignalCustomColor(SettingCustomColorKey key, int index, QVariant entry);
    void SignalDefaultSetting();

private slots:
    void on_heightSpinBox_valueChanged(int arg1);
    void on_positionComboBox_currentIndexChanged(int value);
    void on_transparencySpinBox_valueChanged(int arg1);
    void on_showChargeCheckBox_toggled(bool checked);
    void on_alignComboBox_currentIndexChanged(int value);
    void on_mainMonitorCheckBox_toggled(bool checked);
    void on_customMonitorComboBox_currentIndexChanged(int index);
    void on_defaultColorPushButton_clicked();
    void on_chargeColorPushButton_clicked();
    void on_fullColorPushButton_clicked();
    void on_customEnableComboBox_currentIndexChanged(int index);
    void on_customEnableCheckBox_toggled(bool checked);
    void on_lowEdgeSpinBox_valueChanged(int arg1);
    void on_highEdgeSpinBox_valueChanged(int arg1);
    void on_customColorPushButton_clicked();
    void on_buttonBox_clicked(QAbstractButton *button);

    void reject();
    void done(int ret);

private:
    void SetDefaultOption();
    void UpdateDialog();

    Ui::SettingDialog *ui;
    BL_OPTION m_option;
    BL_OPTION m_first;
    BL_OPTION m_default;
    uint customColorIndex;
};

#endif // SETTINGDIALOG_H
