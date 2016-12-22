#ifndef SETTINGDIALOG_H
#define SETTINGDIALOG_H

#include <QDialog>
#include <QSettings>

#include <cstdint>



enum class SettingGeneralKey { Height = 0, Position = 1, Transparency = 2, ShowCharge = 3, TaskBar = 4, Monitor = 5};
enum class SettingBasicColorKey { DefaultColor = 0, ChargeColor = 1, FullColor = 2 };
enum class SettingCustomColorKey { LowEdge = 0, HighEdge = 1, Color = 2 };
enum class SettingMonitor { Primary = 0, Custom = 1 };
enum class SettingPosition { Top = 1, Bottom = 2, Left = 3, Right = 4 };
enum class SettingTaskBar { Ignore = 0, Evade = 1 };

#define BL_COLOR_LEVEL      8
struct bl_option
{
    uint32_t height;			// Battery line's height (in pixel)
    uint32_t position;		// Where to show battery line? (TOP | BOTTOM | LEFT | RIGHT)
    uint32_t transparency;	// Transparency of battery line
    bool showCharge;		// Show battery line when charging?
    uint32_t taskbar;		// Evade or not if battery line is overlapped with taskbar
    bool mainMonitor;		// Which monitor to show battery line?
    uint32_t customMonitor;		// Which monitor to show battery line?
    QColor defaultColor;	// Battery line's default color
    QColor chargeColor;	// Battery line's color when charging
    QColor fullColor;		// Battery line's color when charging is done
    uint32_t customColorCount;                 // User defined battery line's color count
    uint32_t lowEdge[BL_COLOR_LEVEL];	// User defined edges to pick up user defined color
    uint32_t highEdge[BL_COLOR_LEVEL];	// User defined edges to pick up user defined color
    QColor customColor[BL_COLOR_LEVEL];	// User defined battery line's color

};
typedef struct bl_option BL_OPTION;

struct bl_moninfo
{
    // Resoultion
    uint32_t res_x;
    uint32_t res_y;
    // Virtual Coordinate
    int32_t left;
    int32_t top;
    int32_t right;
    int32_t bottom;
};
typedef struct bl_moninfo BL_MONINFO;

namespace Ui {
    class SettingDialog;
}

class SettingDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingDialog(QWidget *parent = 0);
    ~SettingDialog();

signals:
    void SignalGeneral(SettingGeneralKey key, QVariant entry);
    void SignalBasicColor(SettingBasicColorKey key, QVariant entry);
    void SignalCustomColor(SettingCustomColorKey key, int index, QVariant entry);

private slots:
    void on_heightSpinBox_valueChanged(int arg1);

    void on_positionComboBox_currentIndexChanged(const QString &arg1);

    void on_transparencySpinBox_valueChanged(int arg1);

    void on_showChargeCheckBox_toggled(bool checked);

    void on_taskbarComboBox_currentIndexChanged(const QString &arg1);

    void on_mainMonitorRadioButton_toggled(bool checked);

    void on_curstomMonitorRadioButton_toggled(bool checked);

    void on_customMonitorComboBox_currentIndexChanged(const QString &arg1);

private:
    Ui::SettingDialog *ui;
};

#endif // SETTINGDIALOG_H
