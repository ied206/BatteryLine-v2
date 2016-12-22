#include "settingdialog.h"
#include "ui_settingdialog.h"

SettingDialog::SettingDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingDialog)
{
    ui->setupUi(this);
}

SettingDialog::~SettingDialog()
{
    delete ui;
}

void SettingDialog::on_heightSpinBox_valueChanged(int value)
{
    emit SignalGeneral(SettingGeneralKey::Height, value);
}

void SettingDialog::on_positionComboBox_currentIndexChanged(const QString &value)
{
    if (value.compare(tr("Top"), Qt::CaseInsensitive) == 0)
        emit SignalGeneral(SettingGeneralKey::Position, static_cast<int>(SettingPosition::Top));
    else if (value.compare(tr("Bottom"), Qt::CaseInsensitive) == 0)
        emit SignalGeneral(SettingGeneralKey::Position, static_cast<int>(SettingPosition::Bottom));
    else if (value.compare(tr("Left"), Qt::CaseInsensitive) == 0)
        emit SignalGeneral(SettingGeneralKey::Position, static_cast<int>(SettingPosition::Left));
    else if (value.compare(tr("Right"), Qt::CaseInsensitive) == 0)
        emit SignalGeneral(SettingGeneralKey::Position, static_cast<int>(SettingPosition::Right));
}

void SettingDialog::on_transparencySpinBox_valueChanged(int value)
{
    emit SignalGeneral(SettingGeneralKey::Transparency, value);
}

void SettingDialog::on_showChargeCheckBox_toggled(bool checked)
{
    emit SignalGeneral(SettingGeneralKey::ShowCharge, checked);
}

void SettingDialog::on_taskbarComboBox_currentIndexChanged(const QString &value)
{
    if (value.compare(tr("Evade"), Qt::CaseInsensitive) == 0)
        emit SignalGeneral(SettingGeneralKey::TaskBar, static_cast<int>(SettingTaskBar::Evade));
    else if (value.compare(tr("Ignore"), Qt::CaseInsensitive) == 0)
        emit SignalGeneral(SettingGeneralKey::TaskBar, static_cast<int>(SettingTaskBar::Ignore));
}


void SettingDialog::on_mainMonitorRadioButton_toggled(bool checked)
{
    (void) checked;
    emit SignalGeneral(SettingGeneralKey::Monitor, static_cast<int>(SettingMonitor::Primary));
}

void SettingDialog::on_curstomMonitorRadioButton_toggled(bool checked)
{
    (void) checked;
    emit SignalGeneral(SettingGeneralKey::Monitor, static_cast<int>(SettingMonitor::Custom));
}

void SettingDialog::on_customMonitorComboBox_currentIndexChanged(const QString &value)
{

}
