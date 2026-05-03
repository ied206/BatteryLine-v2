#include "var.h"

#include "settingdialog.h"
#include "ui_settingdialog.h"
#include "systemhelper.h"

#include <QColorDialog>
#include <QAbstractButton>
#include <QDialogButtonBox>
#include <QDebug>
#include <QMessageBox>
#include <QScreen>
#include <QList>
#include <QSignalBlocker>

SettingDialog::SettingDialog(BL_OPTION option, BL_OPTION defaultOption, QWidget *parent):
    QDialog(parent),
    ui(new Ui::SettingDialog)
{
    ui->setupUi(this);

    m_option = option;
    m_first = option;
    m_default = defaultOption;

    setWindowIcon(QIcon(BL_ICON));
    m_customColorIndex = 0;
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
    if (index < 0)
        return;

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
    if (index < 0 || BL_COLOR_LEVEL <= index)
        return;

    // CustomColor
    m_customColorIndex = index;
    ui->customEnableComboBox->setCurrentIndex(static_cast<int>(m_customColorIndex));
    ui->customEnableCheckBox->setChecked(m_option.customEnable[m_customColorIndex]);

    ui->lowEdgeSpinBox->setEnabled(m_option.customEnable[m_customColorIndex]);
    ui->highEdgeSpinBox->setEnabled(m_option.customEnable[m_customColorIndex]);
    ui->customColorPushButton->setEnabled(m_option.customEnable[m_customColorIndex]);

    ui->lowEdgeSpinBox->setValue(static_cast<int>(m_option.lowEdge[m_customColorIndex]));
    ui->highEdgeSpinBox->setValue(static_cast<int>(m_option.highEdge[m_customColorIndex]));
    if (m_option.customEnable[m_customColorIndex])
        ui->customColorPushButton->setText("(" + SystemHelper::RGB_QColorToQString(m_option.customColor[m_customColorIndex]) + ")");
    else
        ui->customColorPushButton->setText("Disabled");
    ui->customColorPushButton->setPalette(m_option.customColor[m_customColorIndex]);
}

void SettingDialog::on_customEnableCheckBox_toggled(bool checked)
{
    if (m_customColorIndex < 0 || BL_COLOR_LEVEL <= m_customColorIndex)
        return;

    m_option.customEnable[m_customColorIndex] = checked;
    emit SignalCustomColor(SettingCustomColorKey::Enable, m_customColorIndex, checked);

    ui->lowEdgeSpinBox->setEnabled(checked);
    ui->highEdgeSpinBox->setEnabled(checked);
    if (m_option.customEnable[m_customColorIndex])
        ui->customColorPushButton->setText("(" + SystemHelper::RGB_QColorToQString(m_option.customColor[m_customColorIndex]) + ")");
    else
        ui->customColorPushButton->setText("Disabled");
    ui->customColorPushButton->setEnabled(checked);
}

void SettingDialog::on_lowEdgeSpinBox_valueChanged(int value)
{
    if (m_customColorIndex < 0 || BL_COLOR_LEVEL <= m_customColorIndex)
        return;

    // Check validity when dialog is closed
    if (m_option.customEnable[m_customColorIndex])
        m_option.lowEdge[m_customColorIndex] = value;
}

void SettingDialog::on_highEdgeSpinBox_valueChanged(int value)
{
    if (m_customColorIndex < 0 || BL_COLOR_LEVEL <= m_customColorIndex)
        return;

    // Check validity when dialog is closed
    if (m_option.customEnable[m_customColorIndex])
        m_option.highEdge[m_customColorIndex] = value;
}

void SettingDialog::on_customColorPushButton_clicked()
{
    if (m_customColorIndex < 0 || BL_COLOR_LEVEL <= m_customColorIndex)
        return;

    QColor color = QColorDialog::getColor(m_option.customColor[m_customColorIndex], this);
    if (color.isValid())
    {
        m_option.customColor[m_customColorIndex] = color;
        ui->customColorPushButton->setText("(" + SystemHelper::RGB_QColorToQString(m_option.customColor[m_customColorIndex]) + ")");
        ui->customColorPushButton->setPalette(m_option.customColor[m_customColorIndex]);
        emit SignalCustomColor(SettingCustomColorKey::Color, m_customColorIndex, color);
    }
}

void SettingDialog::on_buttonBox_clicked(QAbstractButton *button)
{
    if (ui->buttonBox->standardButton(button) == QDialogButtonBox::Reset)
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

    {
        const QSignalBlocker blocker(ui->customMonitorComboBox);
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
        int monitorIndex = screenCount == 0 ? -1 : qBound(0, m_option.customMonitor, screenCount - 1);
        ui->customMonitorComboBox->setCurrentIndex(monitorIndex);
    }

    // BasicColor
    ui->defaultColorPushButton->setText("(" + SystemHelper::RGB_QColorToQString(m_option.defaultColor) + ")");
    ui->defaultColorPushButton->setPalette(m_option.defaultColor);
    ui->chargeColorPushButton->setText("(" + SystemHelper::RGB_QColorToQString(m_option.chargeColor) + ")");
    ui->chargeColorPushButton->setPalette(m_option.chargeColor);
    ui->fullColorPushButton->setText("(" + SystemHelper::RGB_QColorToQString(m_option.fullColor) + ")");
    ui->fullColorPushButton->setPalette(m_option.fullColor);

    // CustomColor
    m_customColorIndex = qBound(0, m_customColorIndex, BL_COLOR_LEVEL - 1);
    ui->customEnableComboBox->setCurrentIndex(m_customColorIndex);
    ui->customEnableCheckBox->setChecked(m_option.customEnable[m_customColorIndex]);

    ui->lowEdgeSpinBox->setEnabled(m_option.customEnable[m_customColorIndex]);
    ui->highEdgeSpinBox->setEnabled(m_option.customEnable[m_customColorIndex]);
    ui->customColorPushButton->setEnabled(m_option.customEnable[m_customColorIndex]);

    ui->lowEdgeSpinBox->setKeyboardTracking(false);
    ui->lowEdgeSpinBox->setValue(m_option.lowEdge[m_customColorIndex]);
    ui->highEdgeSpinBox->setKeyboardTracking(false);
    ui->highEdgeSpinBox->setValue(m_option.highEdge[m_customColorIndex]);
    if (m_option.customEnable[m_customColorIndex])
        ui->customColorPushButton->setText("(" + SystemHelper::RGB_QColorToQString(m_option.customColor[m_customColorIndex]) + ")");
    else
        ui->customColorPushButton->setText("Disabled");
    ui->customColorPushButton->setPalette(m_option.customColor[m_customColorIndex]);

}

void SettingDialog::reject()
{
    QDialog::reject();
}


void SettingDialog::done(int ret)
{
    for (int i = 0; i < BL_COLOR_LEVEL; i++)
    {
        m_option.lowEdge[i] = qBound(0, m_option.lowEdge[i], 100);
        m_option.highEdge[i] = qBound(0, m_option.highEdge[i], 100);
    }

    // Check if (lowEdge < highEdge)
    for (int i = 0; i < BL_COLOR_LEVEL; i++)
    {
        if (m_option.customEnable[i])
        {
            if (m_option.lowEdge[i] < m_option.highEdge[i])
            {
                emit SignalCustomColor(SettingCustomColorKey::LowEdge, i, m_option.lowEdge[i]);
                emit SignalCustomColor(SettingCustomColorKey::HighEdge, i, m_option.highEdge[i]);
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

                m_customColorIndex = i;
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
            int x, overlapStart = i, overlapEnd = 100;
            for (x = i + 1; x < 100; x++)
            {
                if (covered[x] < 2)
                {
                    overlapEnd = x;
                    break;
                }
            }

            int overlapStartIndex = -1, overlapEndIndex = -1;
            for (x = 0; x < BL_COLOR_LEVEL; x++)
            {
                if (!m_option.customEnable[x])
                    continue;

                if (m_option.lowEdge[x] <= overlapStart && overlapStart < m_option.highEdge[x])
                {
                    if (overlapStartIndex == -1)
                        overlapStartIndex = x;
                    else
                    {
                        overlapEndIndex = x;
                        break;
                    }
                }
            }

            if (overlapEndIndex == -1)
                overlapEndIndex = overlapStartIndex;
            if (overlapStartIndex == -1)
            {
                QDialog::done(ret);
                return;
            }

            if (ret == QDialog::Accepted)
            {
                QMessageBox msgBox;
                msgBox.setWindowIcon(QIcon(BL_ICON));
                msgBox.setWindowTitle(tr("Custom Color Error"));
                msgBox.setText(QString("Threshold overlapped from %1 to %2!\nCheck Threshold %3 and %4.").arg(overlapStart).arg(overlapEnd).arg(overlapStartIndex + 1).arg(overlapEndIndex + 1));
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
                    m_customColorIndex = x;
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
