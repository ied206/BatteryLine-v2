#include "var.h"

#include "settingdialog.h"
#include "ui_settingdialog.h"
#include "systemhelper.h"

#include <QDesktopWidget>
#include <QColorDialog>
#include <QAbstractButton>
#include <QDebug>
#include <QMessageBox>
#include <QScreen>
#include <QList>

SettingDialog::SettingDialog(BL_OPTION option, BL_OPTION defaultOption, QWidget *parent):
    QDialog(parent),
    ui(new Ui::SettingDialog)
{
    ui->setupUi(this);

    m_option = option;
    m_first = option;
    m_default = defaultOption;

    setWindowIcon(QIcon(BL_ICON));
    customColorIndex = 0;
    UpdateDialog();
}

SettingDialog::~SettingDialog()
{
    delete ui;
}

void SettingDialog::on_heightSpinBox_valueChanged(int value)
{
    emit SignalGeneral(SettingGeneralKey::Height, value);
}

void SettingDialog::on_positionComboBox_currentIndexChanged(int index)
{
    emit SignalGeneral(SettingGeneralKey::Position, index);
}

void SettingDialog::on_transparencySpinBox_valueChanged(int value)
{
    emit SignalGeneral(SettingGeneralKey::Transparency, value);
}

void SettingDialog::on_showChargeCheckBox_toggled(bool checked)
{
    emit SignalGeneral(SettingGeneralKey::ShowCharge, checked);
}

void SettingDialog::on_alignComboBox_currentIndexChanged(int index)
{
    emit SignalGeneral(SettingGeneralKey::Align, index);
}

void SettingDialog::on_mainMonitorCheckBox_toggled(bool checked)
{
    ui->customMonitorComboBox->setDisabled(checked);
    emit SignalGeneral(SettingGeneralKey::MainMonitor, checked);
}

void SettingDialog::on_customMonitorComboBox_currentIndexChanged(int index)
{
    emit SignalGeneral(SettingGeneralKey::CustomMonitor, index);
}

void SettingDialog::on_defaultColorPushButton_clicked()
{
    QColor color = QColorDialog::getColor(m_option.defaultColor, this);
    if (color.isValid())
    {
        m_option.defaultColor = color;
        ui->defaultColorPushButton->setText("(" + SystemHelper::RGB_QColorToQString(m_option.defaultColor) + ")");
        ui->defaultColorPushButton->setPalette(m_option.defaultColor);
        emit SignalBasicColor(SettingBasicColorKey::DefaultColor, color);
    }
}

void SettingDialog::on_chargeColorPushButton_clicked()
{
    QColor color = QColorDialog::getColor(m_option.chargeColor, this);
    if (color.isValid())
    {
        m_option.chargeColor = color;
        ui->chargeColorPushButton->setText("(" + SystemHelper::RGB_QColorToQString(m_option.chargeColor) + ")");
        ui->chargeColorPushButton->setPalette(m_option.chargeColor);
        emit SignalBasicColor(SettingBasicColorKey::ChargeColor, color);
    }
}

void SettingDialog::on_fullColorPushButton_clicked()
{
    QColor color = QColorDialog::getColor(m_option.fullColor, this);
    if (color.isValid())
    {
        m_option.fullColor = color;
        ui->fullColorPushButton->setText("(" + SystemHelper::RGB_QColorToQString(m_option.fullColor) + ")");
        ui->fullColorPushButton->setPalette(m_option.fullColor);
        emit SignalBasicColor(SettingBasicColorKey::FullColor, color);
    }
}

void SettingDialog::on_customEnableComboBox_currentIndexChanged(int index)
{
    // CustomColor
    customColorIndex = index;
    ui->customEnableComboBox->setCurrentIndex(static_cast<int>(customColorIndex));
    ui->customEnableCheckBox->setChecked(m_option.customEnable[customColorIndex]);

    ui->lowEdgeSpinBox->setEnabled(m_option.customEnable[customColorIndex]);
    ui->highEdgeSpinBox->setEnabled(m_option.customEnable[customColorIndex]);
    ui->customColorPushButton->setEnabled(m_option.customEnable[customColorIndex]);

    ui->lowEdgeSpinBox->setValue(static_cast<int>(m_option.lowEdge[customColorIndex]));
    ui->highEdgeSpinBox->setValue(static_cast<int>(m_option.highEdge[customColorIndex]));
    if (m_option.customEnable[customColorIndex])
        ui->customColorPushButton->setText("(" + SystemHelper::RGB_QColorToQString(m_option.customColor[customColorIndex]) + ")");
    else
        ui->customColorPushButton->setText("Disabled");
    ui->customColorPushButton->setPalette(m_option.customColor[customColorIndex]);
}

void SettingDialog::on_customEnableCheckBox_toggled(bool checked)
{
    m_option.customEnable[customColorIndex] = checked;
    emit SignalCustomColor(SettingCustomColorKey::Enable, customColorIndex, checked);

    ui->lowEdgeSpinBox->setEnabled(checked);
    ui->highEdgeSpinBox->setEnabled(checked);
    if (m_option.customEnable[customColorIndex])
        ui->customColorPushButton->setText("(" + SystemHelper::RGB_QColorToQString(m_option.customColor[customColorIndex]) + ")");
    else
        ui->customColorPushButton->setText("Disabled");
    ui->customColorPushButton->setEnabled(checked);
}

void SettingDialog::on_lowEdgeSpinBox_valueChanged(int value)
{
    // Check validity when dialog is closed
    if (m_option.customEnable[customColorIndex])
        m_option.lowEdge[customColorIndex] = value;
}

void SettingDialog::on_highEdgeSpinBox_valueChanged(int value)
{
    // Check validity when dialog is closed
    if (m_option.customEnable[customColorIndex])
        m_option.highEdge[customColorIndex] = value;
}

void SettingDialog::on_customColorPushButton_clicked()
{
    QColor color = QColorDialog::getColor(m_option.customColor[customColorIndex], this);
    if (color.isValid())
    {
        m_option.customColor[customColorIndex] = color;
        ui->customColorPushButton->setPalette(m_option.customColor[customColorIndex]);
        emit SignalCustomColor(SettingCustomColorKey::Color, customColorIndex, color);
    }
}

void SettingDialog::on_buttonBox_clicked(QAbstractButton *button)
{
    QString text = button->text();
    if (text == "Reset")
    {
        m_option = m_default;
        emit SignalDefaultSetting();
        UpdateDialog();
    }
}

void SettingDialog::UpdateDialog()
{
    // General
    ui->heightSpinBox->setValue(m_option.height);
    ui->positionComboBox->setCurrentIndex(m_option.position);
    ui->transparencySpinBox->setKeyboardTracking(false);
    ui->transparencySpinBox->setValue(m_option.transparency);
    ui->showChargeCheckBox->setChecked(m_option.showCharge);
    ui->alignComboBox->setCurrentIndex(m_option.align);
    ui->mainMonitorCheckBox->setChecked(m_option.mainMonitor);

    if (m_option.mainMonitor)
        ui->customMonitorComboBox->setEnabled(false);
    else
        ui->customMonitorComboBox->setEnabled(true);

    QList<QScreen*> screens = QGuiApplication::screens();
    int screenCount = screens.size();

    ui->customMonitorComboBox->clear();
    for (int i = 0; i < screenCount; i++)
    {
        QScreen* screen = screens.at(i);
        QRect screenRect = screen->geometry();
        ui->customMonitorComboBox->addItem(QString("Monitor %1 (%2x%3)")
                                           .arg(i + 1)
                                           .arg(screenRect.width()).
                                           arg(screenRect.height()));
    }
    ui->customMonitorComboBox->setCurrentIndex(m_option.customMonitor);

    // BasicColor
    ui->defaultColorPushButton->setText("(" + SystemHelper::RGB_QColorToQString(m_option.defaultColor) + ")");
    ui->defaultColorPushButton->setPalette(m_option.defaultColor);
    ui->chargeColorPushButton->setText("(" + SystemHelper::RGB_QColorToQString(m_option.chargeColor) + ")");
    ui->chargeColorPushButton->setPalette(m_option.chargeColor);
    ui->fullColorPushButton->setText("(" + SystemHelper::RGB_QColorToQString(m_option.fullColor) + ")");
    ui->fullColorPushButton->setPalette(m_option.fullColor);

    // CustomColor
    ui->customEnableComboBox->setCurrentIndex(customColorIndex);
    ui->customEnableCheckBox->setChecked(m_option.customEnable[customColorIndex]);

    ui->lowEdgeSpinBox->setEnabled(m_option.customEnable[customColorIndex]);
    ui->highEdgeSpinBox->setEnabled(m_option.customEnable[customColorIndex]);
    ui->customColorPushButton->setEnabled(m_option.customEnable[customColorIndex]);

    ui->lowEdgeSpinBox->setKeyboardTracking(false);
    ui->lowEdgeSpinBox->setValue(m_option.lowEdge[customColorIndex]);
    ui->highEdgeSpinBox->setKeyboardTracking(false);
    ui->highEdgeSpinBox->setValue(m_option.highEdge[customColorIndex]);
    if (m_option.customEnable[customColorIndex])
        ui->customColorPushButton->setText("(" + SystemHelper::RGB_QColorToQString(m_option.customColor[customColorIndex]) + ")");
    else
        ui->customColorPushButton->setText("Disabled");
    ui->customColorPushButton->setPalette(m_option.customColor[customColorIndex]);

}

void SettingDialog::reject()
{
    QDialog::reject();
}


void SettingDialog::done(int ret)
{
    // Check if (lowEdge < highEdge)
    for (int i = 0; i < BL_COLOR_LEVEL; i++)
    {
        if (m_option.customEnable[i])
        {
            if (m_option.lowEdge[i] < m_option.highEdge[i])
            {
                emit SignalCustomColor(SettingCustomColorKey::LowEdge, customColorIndex, ui->lowEdgeSpinBox->value());
                emit SignalCustomColor(SettingCustomColorKey::HighEdge, customColorIndex, ui->highEdgeSpinBox->value());
            }
            else // if (m_option.lowEdge[i] != 0 && m_option.highEdge[i] != 0)
            {
                QMessageBox msgBox;
                msgBox.setWindowIcon(QIcon(BL_ICON));
                msgBox.setWindowTitle(tr("Custom Color Error"));
                msgBox.setText(QString("Threshold %1's LowEdge (%2) cannot be larger than HighEdge (%3).\nThreshold %1 will be disabled.").arg(i + 1).arg(m_option.lowEdge[i]).arg(m_option.highEdge[i]));
                msgBox.setIcon(QMessageBox::Critical);
                msgBox.setStandardButtons(QMessageBox::Ok);
                msgBox.setDefaultButton(QMessageBox::Ok);
                msgBox.exec();

                customColorIndex = i;
                m_option.customEnable[i] = false;
                UpdateDialog();

                return;
            }
        }
    }

    // Check overlap of lowEdges and highEdges
    uint8_t covered[100] = { 0 };
    for (int i = 0; i < BL_COLOR_LEVEL; i++)
    {
        if (m_option.customEnable[i])
        {
            for (int x = m_option.lowEdge[i]; x < m_option.highEdge[i]; x++)
                covered[x]++;
        }
    }

    for (int i = 0; i < 100; i++)
    {
        if (1 < covered[i])
        { // Overlap detected!
            int x, overlapStart = i, overlapEnd = i;
            for (x = i + 1; x < 100; x++)
            {
                if (covered[x] == 1)
                {
                    overlapEnd = x;
                    break;
                }
            }

            int overlapStartIndex = 0, overlapEndIndex = 0;
            for (x = 0; x < BL_COLOR_LEVEL; x++)
            {
                if (m_option.lowEdge[x] < overlapStart && overlapStart < m_option.highEdge[x])
                    overlapStartIndex = x;
                else if (m_option.lowEdge[x] < overlapEnd && overlapEnd < m_option.highEdge[x])
                    overlapEndIndex = x;
            }

            if (ret == QDialog::Accepted)
            {
                QMessageBox msgBox;
                msgBox.setWindowIcon(QIcon(BL_ICON));
                msgBox.setWindowTitle(tr("Custom Color Error"));
                msgBox.setText(QString("Threshold overlapped from %1 to %2!\nCheck Threshold %3 and %4.").arg(overlapStart).arg(overlapEnd).arg(overlapStartIndex).arg(overlapEndIndex));
                msgBox.setIcon(QMessageBox::Critical);
                msgBox.setStandardButtons(QMessageBox::Ok);
                msgBox.setDefaultButton(QMessageBox::Ok);
                msgBox.exec();
            }

            // Set invalid value to last value
            for (x = 0; x < BL_COLOR_LEVEL; x++)
            {
                if (x == overlapStartIndex)
                {
                    customColorIndex = x;
                    m_option.highEdge[x] = m_option.lowEdge[overlapEndIndex];
                }
            }
            UpdateDialog();

            switch (ret)
            {
            case QDialog::Accepted:
                return;
            case QDialog::Rejected:
                QDialog::done(ret);
                break;
            }
        }
    }

    QDialog::done(ret);
}
