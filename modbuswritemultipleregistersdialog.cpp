#include "modbuswritemultipleregistersdialog.h"
#include "ui_modbuswritemultipleregistersdialog.h"
#include "regsviewwidget.h"
#include "utils.h"
#include "modbus_rtu.h"
#include "modbus_ascii.h"
#include "modbus_tcp.h"
#include "openroutedialog.h"
#include "modbuswidget.h"
#include <QMessageBox>
#include <QtEndian>
#include <QInputDialog>

using CellFormat = RegsViewWidget::CellFormat;

const QMap<QString, int> ModbusWriteMultipleRegistersDialog::format_map = {
    {tr("Signed"), CellFormat::Format_Signed},
    {tr("Unsigned"), CellFormat::Format_Unsigned},
    {tr("Hex"), CellFormat::Format_Hex},
    {tr("Binary"), CellFormat::Format_Binary},
    {tr("Int32 Big-endian"), CellFormat::Format_32_Bit_Signed_Big_Endian},
    {tr("Int32 Little-endian"), CellFormat::Format_32_Bit_Signed_Little_Endian},
    {tr("Int32 Big-endian byte swap"), CellFormat::Format_32_Bit_Signed_Big_Endian_Byte_Swap},
    {tr("Int32 Little-endian byte swap"), CellFormat::Format_32_Bit_Signed_Little_Endian_Byte_Swap},
    {tr("UInt32 Big-endian"), CellFormat::Format_32_Bit_Unsigned_Big_Endian},
    {tr("UInt32 Little-endian"), CellFormat::Format_32_Bit_Unsigned_Little_Endian},
    {tr("UInt32 Big-endian byte swap"), CellFormat::Format_32_Bit_Unsigned_Big_Endian_Byte_Swap},
    {tr("UInt32 Little-endian byte swap"), CellFormat::Format_32_Bit_Unsigned_Little_Endian_Byte_Swap},
    {tr("Int64 Big-endian"), CellFormat::Format_64_Bit_Signed_Big_Endian},
    {tr("Int64 Little-endian"), CellFormat::Format_64_Bit_Signed_Little_Endian},
    {tr("Int64 Big-endian byte swap"), CellFormat::Format_64_Bit_Signed_Big_Endian_Byte_Swap},
    {tr("Int64 Little-endian byte swap"), CellFormat::Format_64_Bit_Signed_Little_Endian_Byte_Swap},
    {tr("UInt64 Big-endian"), CellFormat::Format_64_Bit_Unsigned_Big_Endian},
    {tr("UInt64 Little-endian"), CellFormat::Format_64_Bit_Unsigned_Little_Endian},
    {tr("UInt64 Big-endian byte swap"), CellFormat::Format_64_Bit_Unsigned_Big_Endian_Byte_Swap},
    {tr("UInt64 Little-endian byte swap"), CellFormat::Format_64_Bit_Unsigned_Little_Endian_Byte_Swap},
    {tr("Float Big-endian"), CellFormat::Format_32_Bit_Float_Big_Endian},
    {tr("Float Little-endian"), CellFormat::Format_32_Bit_Float_Little_Endian},
    {tr("Float Big-endian byte swap"), CellFormat::Format_32_Bit_Float_Big_Endian_Byte_Swap},
    {tr("Float Little-endian byte swap"), CellFormat::Format_32_Bit_Float_Little_Endian_Byte_Swap},
    {tr("Double Big-endian"), CellFormat::Format_64_Bit_Float_Big_Endian},
    {tr("Double Little-endian"), CellFormat::Format_64_Bit_Float_Little_Endian},
    {tr("Double Big-endian byte swap"), CellFormat::Format_64_Bit_Float_Big_Endian_Byte_Swap},
    {tr("Double Little-endian byte swap"), CellFormat::Format_64_Bit_Float_Little_Endian_Byte_Swap}
};

ModbusWriteMultipleRegistersDialog::ModbusWriteMultipleRegistersDialog(int protocol, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ModbusWriteMultipleRegistersDialog), m_protocol(protocol)
{
    ui->setupUi(this);
    setWindowModality(Qt::WindowModal);
    setWindowTitle(tr("16:Write Multiple Registers"));
    m_format = CellFormat::Format_Signed;
    m_reg_values = new quint16[1]{0};
    ui->box_type->addItems(format_map.keys());
    ui->box_type->setCurrentIndex(0);

}

ModbusWriteMultipleRegistersDialog::~ModbusWriteMultipleRegistersDialog()
{
    delete ui;
}

void ModbusWriteMultipleRegistersDialog::responseSlot(int error_code)
{
    if(!isVisible())
    {
        return;
    }
    QString tip_string;
    if(error_code == ModbusErrorCode_OK)
    {
        tip_string = tr("Response ok");
    }
    else if(error_code == ModbusErrorCode_Timeout)
    {
        tip_string = tr("Timeout error");
    }
    else
    {
        tip_string = ModbusWidget::modbus_error_code_map[(ModbusErrorCode)error_code];
    }
    QMessageBox::warning(this, "Modbus", tip_string);
}

void ModbusWriteMultipleRegistersDialog::on_box_quantity_valueChanged(int arg1)
{
    ui->list_values->setDisabled(true);
    ui->button_send->setDisabled(true);
    int reg_quantity = ui->box_quantity->value();
    delete[] m_reg_values;
    m_reg_values = new quint16[reg_quantity]{0};
}


void ModbusWriteMultipleRegistersDialog::on_button_update_value_list_clicked()
{
    if(update_value_list())
    {
        ui->list_values->setEnabled(true);
        ui->button_send->setEnabled(true); 
    }
}


void ModbusWriteMultipleRegistersDialog::on_box_type_currentTextChanged(const QString &arg1)
{
    m_format = format_map[arg1];
    update_value_list();
}


void ModbusWriteMultipleRegistersDialog::on_list_values_itemDoubleClicked(QListWidgetItem *item)
{
    int unit_size = 0;
    if(m_format < 32)
    {
        unit_size = 1;
    }
    else if(m_format < 64)
    {
        unit_size = 2;
    }
    else
    {
        unit_size = 4;
    }
    int data_index = ui->list_values->row(item) * unit_size;
    bool input_ok{false};
    bool cvt_ok{false};
    switch(m_format)
    {
    case CellFormat::Format_Signed:
    {
        qint16 *value = (qint16*)&m_reg_values[data_index];
        qint16 input_val = QInputDialog::getInt(this, tr("Edit Register"), tr("Value:"), *value, -32768, 32767, 1, &input_ok);
        if(input_ok)
        {
            *value = input_val;
        }
        break;
    }
    case CellFormat::Format_Unsigned:
    {
        quint16 *value = (quint16*)&m_reg_values[data_index];
        quint16 input_val = QInputDialog::getInt(this, tr("Edit Register"), tr("Value:"), *value, 0, 65535, 1, &input_ok);
        if(input_ok)
        {
            *value = input_val;
        }
        break;
    }
    case CellFormat::Format_Hex:
    {
        quint16 *value = (quint16*)&m_reg_values[data_index];
        QString input_val = QInputDialog::getText(this, tr("Edit Register"), tr("Value:"), QLineEdit::Normal, QString::number(*value, 16), &input_ok);
        if(input_ok)
        {
            quint16 new_val = input_val.toUInt(&cvt_ok, 16);
            if(cvt_ok)
            {
                *value = new_val;
            }
        }
        break;
    }
    case CellFormat::Format_Binary:
    {
        quint16 *value = (quint16*)&m_reg_values[data_index];
        QString input_val = QInputDialog::getText(this, tr("Edit Register"), tr("Value:"), QLineEdit::Normal, QString::number(*value, 2), &input_ok);
        if(input_ok)
        {
            quint16 new_val = input_val.toUInt(&cvt_ok, 2);
            if(cvt_ok)
            {
                *value = new_val;
            }
        }
        break;
    }
    case CellFormat::Format_32_Bit_Signed_Big_Endian:
    {
        qint32 value = qFromBigEndian<qint32>(&m_reg_values[data_index]);
        qint32 input_val = QInputDialog::getInt(this, tr("Edit Register"), tr("Value:"), value, -2147483648, 2147483647, 1, &input_ok);
        if(input_ok)
        {
            qToBigEndian<qint32>(input_val,&m_reg_values[data_index]);
        }
        break;
    }
    case CellFormat::Format_32_Bit_Signed_Little_Endian:
    {
        qint32 value = qFromLittleEndian<qint32>(&m_reg_values[data_index]);
        qint32 input_val = QInputDialog::getInt(this, tr("Edit Register"), tr("Value:"), value, -2147483648, 2147483647, 1, &input_ok);
        if(input_ok)
        {
            qToLittleEndian<qint32>(input_val, &m_reg_values[data_index]);
        }
        break;
    }
    case CellFormat::Format_32_Bit_Signed_Big_Endian_Byte_Swap:
    {
        qint32 value = myFromBigEndianByteSwap<qint32>(&m_reg_values[data_index]);
        qint32 input_val = QInputDialog::getInt(this, tr("Edit Register"), tr("value:"), value, -2147483648, 2147483647, 1, &input_ok);
        if(input_ok)
        {
            myToBigEndianByteSwap<qint32>(input_val, &m_reg_values[data_index]);
        }
        break;
    }
    case CellFormat::Format_32_Bit_Signed_Little_Endian_Byte_Swap:
    {
        qint32 value = myFromLittleEndianByteSwap<qint32>(&m_reg_values[data_index]);
        qint32 input_val = QInputDialog::getInt(this, tr("Edit Register"), tr("value:"), value, -2147483648, 2147483647, 1, &input_ok);
        if(input_ok)
        {
            myToLittleEndianByteSwap<qint32>(input_val, &m_reg_values[data_index]);
        }
        break;
    }
    case CellFormat::Format_32_Bit_Unsigned_Big_Endian:
    {
        quint32 value = qFromBigEndian<quint32>(&m_reg_values[data_index]);
        QString input_val = QInputDialog::getText(this, tr("Edit Register"), tr("value:"), QLineEdit::Normal, QString("%1").arg(value), &input_ok);
        if(input_ok)
        {
            quint32 new_val = input_val.toUInt(&cvt_ok);
            if(cvt_ok)
            {
                qToBigEndian<quint32>(new_val, &m_reg_values[data_index]);
            }
        }
        break;
    }
    case CellFormat::Format_32_Bit_Unsigned_Little_Endian:
    {
        quint32 value = qFromLittleEndian<quint32>(&m_reg_values[data_index]);
        QString input_val = QInputDialog::getText(this, tr("Edit Register"), tr("value:"), QLineEdit::Normal, QString("%1").arg(value), &input_ok);
        if(input_ok)
        {
            quint32 new_val = input_val.toUInt(&cvt_ok);
            if(cvt_ok)
            {
                qToLittleEndian<quint32>(new_val, &m_reg_values[data_index]);
            }
        }
        break;
    }
    case CellFormat::Format_32_Bit_Unsigned_Big_Endian_Byte_Swap:
    {
        quint32 value = myFromBigEndianByteSwap<quint32>(&m_reg_values[data_index]);
        QString input_val = QInputDialog::getText(this, tr("Edit Register"), tr("value:"), QLineEdit::Normal, QString("%1").arg(value), &input_ok);
        if(input_ok)
        {
            quint32 new_val = input_val.toUInt(&cvt_ok);
            if(cvt_ok)
            {
                myToBigEndianByteSwap<quint32>(new_val, &m_reg_values[data_index]);
            }
        }
        break;
    }
    case CellFormat::Format_32_Bit_Unsigned_Little_Endian_Byte_Swap:
    {
        quint32 value = myFromLittleEndianByteSwap<quint32>(&m_reg_values[data_index]);
        QString input_val = QInputDialog::getText(this, tr("Edit Register"), tr("value:"), QLineEdit::Normal, QString("%1").arg(value), &input_ok);
        if(input_ok)
        {
            quint32 new_val = input_val.toUInt(&cvt_ok);
            if(cvt_ok)
            {
                myToLittleEndianByteSwap<quint32>(new_val, &m_reg_values[data_index]);
            }
        }
        break;
    }
    case CellFormat::Format_64_Bit_Signed_Big_Endian:
    {
        qint64 value = qFromBigEndian<qint64>(&m_reg_values[data_index]);
        QString input_val = QInputDialog::getText(this, tr("Edit Register"), tr("value:"), QLineEdit::Normal, QString("%1").arg(value), &input_ok);
        if(input_ok)
        {
            qint64 new_val = input_val.toLongLong(&cvt_ok);
            if(cvt_ok)
            {
                qToBigEndian<qint64>(new_val, &m_reg_values[data_index]);
            }
        }
        break;
    }
    case CellFormat::Format_64_Bit_Signed_Little_Endian:
    {
        qint64 value = qFromLittleEndian<qint64>(&m_reg_values[data_index]);
        QString input_val = QInputDialog::getText(this, tr("Edit Register"), tr("value:"), QLineEdit::Normal, QString("%1").arg(value), &input_ok);
        if(input_ok)
        {
            qint64 new_val = input_val.toLongLong(&cvt_ok);
            if(cvt_ok)
            {
                qToLittleEndian<qint64>(new_val, &m_reg_values[data_index]);
            }
        }
        break;
    }
    case CellFormat::Format_64_Bit_Signed_Big_Endian_Byte_Swap:
    {
        qint64 value = myFromBigEndianByteSwap<qint64>(&m_reg_values[data_index]);
        QString input_val = QInputDialog::getText(this, tr("Edit Register"), tr("value:"), QLineEdit::Normal, QString("%1").arg(value), &input_ok);
        if(input_ok)
        {
            qint64 new_val = input_val.toLongLong(&cvt_ok);
            if(cvt_ok)
            {
                myToBigEndianByteSwap<qint64>(new_val, &m_reg_values[data_index]);
            }
        }
        break;
    }
    case CellFormat::Format_64_Bit_Signed_Little_Endian_Byte_Swap:
    {
        qint64 value = myFromLittleEndianByteSwap<qint64>(&m_reg_values[data_index]);
        QString input_val = QInputDialog::getText(this, tr("Edit Register"), tr("value:"), QLineEdit::Normal, QString("%1").arg(value), &input_ok);
        if(input_ok)
        {
            qint64 new_val = input_val.toLongLong(&cvt_ok);
            if(cvt_ok)
            {
                myToLittleEndianByteSwap<qint64>(new_val, &m_reg_values[data_index]);
            }
        }
        break;
    }
    case CellFormat::Format_64_Bit_Unsigned_Big_Endian:
    {
        quint64 value = qFromBigEndian<quint64>(&m_reg_values[data_index]);
        QString input_val = QInputDialog::getText(this, tr("Edit Register"), tr("value:"), QLineEdit::Normal, QString("%1").arg(value), &input_ok);
        if(input_ok)
        {
            quint64 new_val = input_val.toULongLong(&cvt_ok);
            if(cvt_ok)
            {
                qToBigEndian<quint64>(new_val, &m_reg_values[data_index]);
            }
        }
        break;
    }
    case CellFormat::Format_64_Bit_Unsigned_Little_Endian:
    {
        quint64 value = qFromLittleEndian<quint64>(&m_reg_values[data_index]);
        QString input_val = QInputDialog::getText(this, tr("Edit Register"), tr("value:"), QLineEdit::Normal, QString("%1").arg(value), &input_ok);
        if(input_ok)
        {
            quint64 new_val = input_val.toULongLong(&cvt_ok);
            if(cvt_ok)
            {
                qToLittleEndian<quint64>(new_val, &m_reg_values[data_index]);
            }
        }
        break;
    }
    case CellFormat::Format_64_Bit_Unsigned_Big_Endian_Byte_Swap:
    {
        quint64 value = myFromBigEndianByteSwap<quint64>(&m_reg_values[data_index]);
        QString input_val = QInputDialog::getText(this, tr("Edit Register"), tr("value:"), QLineEdit::Normal, QString("%1").arg(value), &input_ok);
        if(input_ok)
        {
            quint64 new_val = input_val.toULongLong(&cvt_ok);
            if(cvt_ok)
            {
                myToBigEndianByteSwap<quint64>(new_val, &m_reg_values[data_index]);
            }
        }
        break;
    }
    case CellFormat::Format_64_Bit_Unsigned_Little_Endian_Byte_Swap:
    {
        quint64 value = myFromLittleEndianByteSwap<quint64>(&m_reg_values[data_index]);
        QString input_val = QInputDialog::getText(this, tr("Edit Register"), tr("value:"), QLineEdit::Normal, QString("%1").arg(value), &input_ok);
        if(input_ok)
        {
            quint64 new_val = input_val.toULongLong(&cvt_ok);
            if(cvt_ok)
            {
                myToLittleEndianByteSwap<quint64>(new_val, &m_reg_values[data_index]);
            }
        }
        break;
    }
    case CellFormat::Format_32_Bit_Float_Big_Endian:
    {
        float value = myFromBigEndianByteSwap<float>(&m_reg_values[data_index]);
        QString input_val = QInputDialog::getText(this, tr("Edit Register"), tr("value:"), QLineEdit::Normal, QString("%1").arg(value), &input_ok);
        if(input_ok)
        {
            float new_val = input_val.toFloat(&cvt_ok);
            if(cvt_ok)
            {
                myToBigEndianByteSwap<float>(new_val, &m_reg_values[data_index]);
            }
        }
        break;
    }
    case CellFormat::Format_32_Bit_Float_Little_Endian:
    {
        float value = myFromLittleEndianByteSwap<float>(&m_reg_values[data_index]);
        QString input_val = QInputDialog::getText(this, tr("Edit Register"), tr("value:"), QLineEdit::Normal, QString("%1").arg(value), &input_ok);
        if(input_ok)
        {
            float new_val = input_val.toFloat(&cvt_ok);
            if(cvt_ok)
            {
                myToLittleEndianByteSwap<float>(new_val, &m_reg_values[data_index]);
            }
        }
        break;
    }
    case CellFormat::Format_32_Bit_Float_Big_Endian_Byte_Swap:
    {
        float value = qFromBigEndian<float>(&m_reg_values[data_index]);
        QString input_val = QInputDialog::getText(this, tr("Edit Register"), tr("value:"), QLineEdit::Normal, QString("%1").arg(value), &input_ok);
        if(input_ok)
        {
            float new_val = input_val.toFloat(&cvt_ok);
            if(cvt_ok)
            {
                qToBigEndian<float>(new_val, &m_reg_values[data_index]);
            }
        }
        break;
    }
    case CellFormat::Format_32_Bit_Float_Little_Endian_Byte_Swap:
    {
        float value = qFromLittleEndian<float>(&m_reg_values[data_index]);
        QString input_val = QInputDialog::getText(this, tr("Edit Register"), tr("value:"), QLineEdit::Normal, QString("%1").arg(value), &input_ok);
        if(input_ok)
        {
            float new_val = input_val.toFloat(&cvt_ok);
            if(cvt_ok)
            {
                qToLittleEndian<float>(new_val, &m_reg_values[data_index]);
            }
        }
        break;
    }
    case CellFormat::Format_64_Bit_Float_Big_Endian:
    {
        double value = myFromBigEndianByteSwap<double>(&m_reg_values[data_index]);
        QString input_val = QInputDialog::getText(this, tr("Edit Register"), tr("value:"), QLineEdit::Normal, QString("%1").arg(value), &input_ok);
        if(input_ok)
        {
            double new_val = input_val.toDouble(&cvt_ok);
            if(cvt_ok)
            {
                myToBigEndianByteSwap<double>(new_val, &m_reg_values[data_index]);
            }
        }
        break;
    }
    case CellFormat::Format_64_Bit_Float_Little_Endian:
    {
        double value = myFromLittleEndianByteSwap<double>(&m_reg_values[data_index]);
        QString input_val = QInputDialog::getText(this, tr("Edit Register"), tr("value:"), QLineEdit::Normal, QString("%1").arg(value), &input_ok);
        if(input_ok)
        {
            double new_val = input_val.toDouble(&cvt_ok);
            if(cvt_ok)
            {
                myToLittleEndianByteSwap<double>(new_val, &m_reg_values[data_index]);
            }
        }
        break;
    }
    case CellFormat::Format_64_Bit_Float_Big_Endian_Byte_Swap:
    {
        double value = qFromBigEndian<double>(&m_reg_values[data_index]);
        QString input_val = QInputDialog::getText(this, tr("Edit Register"), tr("value:"), QLineEdit::Normal, QString("%1").arg(value), &input_ok);
        if(input_ok)
        {
            double new_val = input_val.toDouble(&cvt_ok);
            if(cvt_ok)
            {
                qToBigEndian<double>(new_val, &m_reg_values[data_index]);
            }
        }
        break;
    }
    case CellFormat::Format_64_Bit_Float_Little_Endian_Byte_Swap:
    {
        double value = qFromLittleEndian<double>(&m_reg_values[data_index]);
        QString input_val = QInputDialog::getText(this, tr("Edit Register"), tr("value:"), QLineEdit::Normal, QString("%1").arg(value), &input_ok);
        if(input_ok)
        {
            double new_val = input_val.toDouble(&cvt_ok);
            if(cvt_ok)
            {
                qToLittleEndian<double>(new_val, &m_reg_values[data_index]);
            }
        }
        break;
    }
    default:
    {
        return;
    }
    }
    update_value_list();
}


void ModbusWriteMultipleRegistersDialog::on_button_cancel_clicked()
{
    hide();
}


void ModbusWriteMultipleRegistersDialog::on_button_edit_clicked()
{
    QListWidgetItem *item = ui->list_values->currentItem();
    if(item)
    {
        on_list_values_itemDoubleClicked(item);
    }
}


void ModbusWriteMultipleRegistersDialog::on_button_send_clicked()
{
    ModbusFrameInfo frame_info{};
    QByteArray write_pack;
    frame_info.id = ui->box_slave_id->value();
    frame_info.function = ModbusWriteMultipleRegisters;
    frame_info.reg_addr = ui->box_address->value();
    frame_info.quantity = ui->box_quantity->value();
    for(int i = 0; i < frame_info.quantity;++i)
    {
        frame_info.reg_values[i] = m_reg_values[i];
    }
    switch(m_protocol)
    {
    case MODBUS_RTU:
    {
        write_pack = Modbus_RTU::masterFrame2Pack(frame_info);
        break;
    }
    case MODBUS_ASCII:
    {
        write_pack = Modbus_ASCII::masterFrame2Pack(frame_info);
        break;
    }
    case MODBUS_TCP:
    case MODBUS_UDP:
    {
        write_pack = Modbus_TCP::masterFrame2Pack(frame_info);
        break;
    }
    }
    emit writeFunctionTriggered(write_pack);
}

bool ModbusWriteMultipleRegistersDialog::update_value_list()
{
    int unit_size = 0;
    if(m_format < 32)
    {
        unit_size = 1;
    }
    else if(m_format < 64)
    {
        unit_size = 2;
    }
    else
    {
        unit_size = 4;
    }
    int reg_addr = ui->box_address->value();
    int reg_quantity = ui->box_quantity->value();
    if(reg_quantity < unit_size)
    {
        QMessageBox::warning(this, tr("Warning"), tr("The quantity of registers is less than the format size."));
        return false;
    }
    ui->list_values->clear();
    QString item_text;
    int item_count = reg_quantity / unit_size;
    for(int i = 0; i < item_count; ++i)
    {
        switch(m_format)
        {
        case CellFormat::Format_Signed:
        {
            item_text = QString("%1 = %2").arg(i * unit_size + reg_addr).arg(m_reg_values[i * unit_size]);
            break;
        }
        case CellFormat::Format_Unsigned:
        {
            item_text = QString("%1 = %2").arg(i * unit_size + reg_addr).arg(m_reg_values[i * unit_size]);
            break;
        }
        case CellFormat::Format_Hex:
        {
            item_text = QString("%1 = %2").arg(i * unit_size + reg_addr).arg(m_reg_values[i * unit_size], 4, 16, QChar('0'));
            break;
        }
        case CellFormat::Format_Binary:
        {
            item_text = QString("%1 = %2").arg(i * unit_size + reg_addr).arg(m_reg_values[i * unit_size], 16, 2, QChar('0'));
            break;
        }
        case CellFormat::Format_32_Bit_Signed_Big_Endian:
        {
            item_text = QString("%1 = %2").arg(i * unit_size + reg_addr).arg(qFromBigEndian<qint32>(&m_reg_values[i * unit_size]));
            break;
        }
        case CellFormat::Format_32_Bit_Signed_Little_Endian:
        {
            item_text = QString("%1 = %2").arg(i * unit_size + reg_addr).arg(qFromLittleEndian<qint32>(&m_reg_values[i * unit_size]));
            break;
        }
        case CellFormat::Format_32_Bit_Signed_Big_Endian_Byte_Swap:
        {
            item_text = QString("%1 = %2").arg(i * unit_size + reg_addr).arg(myFromBigEndianByteSwap<qint32>(&m_reg_values[i * unit_size]));
            break;
        }
        case CellFormat::Format_32_Bit_Signed_Little_Endian_Byte_Swap:
        {
            item_text = QString("%1 = %2").arg(i * unit_size + reg_addr).arg(myFromLittleEndianByteSwap<qint32>(&m_reg_values[i * unit_size]));
            break;
        }
        case CellFormat::Format_32_Bit_Unsigned_Big_Endian:
        {
            item_text = QString("%1 = %2").arg(i * unit_size + reg_addr).arg(qFromBigEndian<quint32>(&m_reg_values[i * unit_size]));
            break;
        }
        case CellFormat::Format_32_Bit_Unsigned_Little_Endian:
        {
            item_text = QString("%1 = %2").arg(i * unit_size + reg_addr).arg(qFromLittleEndian<quint32>(&m_reg_values[i * unit_size]));
            break;
        }
        case CellFormat::Format_32_Bit_Unsigned_Big_Endian_Byte_Swap:
        {
            item_text = QString("%1 = %2").arg(i * unit_size + reg_addr).arg(myFromBigEndianByteSwap<quint32>(&m_reg_values[i * unit_size]));
            break;
        }
        case CellFormat::Format_32_Bit_Unsigned_Little_Endian_Byte_Swap:
        {
            item_text = QString("%1 = %2").arg(i * unit_size + reg_addr).arg(myFromLittleEndianByteSwap<quint32>(&m_reg_values[i * unit_size]));
            break;
        }
        case CellFormat::Format_64_Bit_Signed_Big_Endian:
        {
            item_text = QString("%1 = %2").arg(i * unit_size + reg_addr).arg(qFromBigEndian<qint64>(&m_reg_values[i * unit_size]));
            break;
        }
        case CellFormat::Format_64_Bit_Signed_Little_Endian:
        {
            item_text = QString("%1 = %2").arg(i * unit_size + reg_addr).arg(qFromLittleEndian<qint64>(&m_reg_values[i * unit_size]));
            break;
        }
        case CellFormat::Format_64_Bit_Signed_Big_Endian_Byte_Swap:
        {
            item_text = QString("%1 = %2").arg(i * unit_size + reg_addr).arg(myFromBigEndianByteSwap<qint64>(&m_reg_values[i * unit_size]));
            break;
        }
        case CellFormat::Format_64_Bit_Signed_Little_Endian_Byte_Swap:
        {
            item_text = QString("%1 = %2").arg(i * unit_size + reg_addr).arg(myFromLittleEndianByteSwap<qint64>(&m_reg_values[i * unit_size]));
            break;
        }
        case CellFormat::Format_64_Bit_Unsigned_Big_Endian:
        {
            item_text = QString("%1 = %2").arg(i * unit_size + reg_addr).arg(qFromBigEndian<quint64>(&m_reg_values[i * unit_size]));
            break;
        }
        case CellFormat::Format_64_Bit_Unsigned_Little_Endian:
        {
            item_text = QString("%1 = %2").arg(i * unit_size + reg_addr).arg(qFromLittleEndian<quint64>(&m_reg_values[i * unit_size]));
            break;
        }
        case CellFormat::Format_64_Bit_Unsigned_Big_Endian_Byte_Swap:
        {
            item_text = QString("%1 = %2").arg(i * unit_size + reg_addr).arg(myFromBigEndianByteSwap<quint64>(&m_reg_values[i * unit_size]));
            break;
        }
        case CellFormat::Format_64_Bit_Unsigned_Little_Endian_Byte_Swap:
        {
            item_text = QString("%1 = %2").arg(i * unit_size + reg_addr).arg(myFromLittleEndianByteSwap<quint64>(&m_reg_values[i * unit_size]));
            break;
        }
        case CellFormat::Format_32_Bit_Float_Big_Endian:
        {
            item_text = QString("%1 = %2").arg(i * unit_size + reg_addr).arg(myFromBigEndianByteSwap<float>(&m_reg_values[i * unit_size]));
            break;
        }
        case CellFormat::Format_32_Bit_Float_Little_Endian:
        {
            item_text = QString("%1 = %2").arg(i * unit_size + reg_addr).arg(myFromLittleEndianByteSwap<float>(&m_reg_values[i * unit_size]));
            break;
        }
        case CellFormat::Format_32_Bit_Float_Big_Endian_Byte_Swap:
        {
            item_text = QString("%1 = %2").arg(i * unit_size + reg_addr).arg(qFromBigEndian<float>(&m_reg_values[i * unit_size]));
            break;
        }
        case CellFormat::Format_32_Bit_Float_Little_Endian_Byte_Swap:
        {
            item_text = QString("%1 = %2").arg(i * unit_size + reg_addr).arg(qFromLittleEndian<float>(&m_reg_values[i * unit_size]));
            break;
        }
        case CellFormat::Format_64_Bit_Float_Big_Endian:
        {
            item_text = QString("%1 = %2").arg(i * unit_size + reg_addr).arg(myFromBigEndianByteSwap<double>(&m_reg_values[i * unit_size]));
            break;
        }
        case CellFormat::Format_64_Bit_Float_Little_Endian:
        {
            item_text = QString("%1 = %2").arg(i * unit_size + reg_addr).arg(myFromLittleEndianByteSwap<double>(&m_reg_values[i * unit_size]));
            break;
        }
        case CellFormat::Format_64_Bit_Float_Big_Endian_Byte_Swap:
        {
            item_text = QString("%1 = %2").arg(i * unit_size + reg_addr).arg(qFromBigEndian<double>(&m_reg_values[i * unit_size]));
            break;
        }
        case CellFormat::Format_64_Bit_Float_Little_Endian_Byte_Swap:
        {
            item_text = QString("%1 = %2").arg(i * unit_size + reg_addr).arg(qFromLittleEndian<double>(&m_reg_values[i * unit_size]));
            break;
        }
        default:
            break;
        }
        ui->list_values->addItem(item_text);
    }
    return true;
}

