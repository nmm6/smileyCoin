//
// Created by Lenovo on 6/15/2020.
//

#include "editservicedialog.h"
#include "ui_editservicedialog.h"

#include "walletmodel.h"
#include "bitcoingui.h"
#include "bitcoinunits.h"
#include "guiutil.h"
#include "util.h"
#include "base58.h"
#include "coincontroldialog.h"
#include "coincontrol.h"
#include "optionsmodel.h"
#include "sendcoinsentry.h"
#include "sendcoinsdialog.h"
#include "init.h"
#include "servicelistdb.h"

#include <qvalidatedlineedit.h>
#include <wallet.h>


EditServiceDialog::EditServiceDialog(Mode mode, QWidget *parent) :
        QDialog(parent),
        ui(new Ui::EditServiceDialog),
        mode(mode),
        model(0)
{
    ui->setupUi(this);

    switch(mode)
    {
        case NewService:
        {
            ui->ticketForm->hide();
            ui->serviceForm->show();
            setWindowTitle(tr("Create new service"));
            ui->serviceType->addItem("Ticket Sales");
            ui->serviceType->addItem("UBI");
            ui->serviceType->addItem("Book Chapter");
            ui->serviceType->addItem("Traceability");
            ui->serviceType->addItem("NPO");
            ui->serviceType->addItem("DEX");

            break;
        }
        case DeleteService:
        {
            ui->ticketForm->hide();
            ui->serviceForm->hide();
            setWindowTitle(tr("Delete service"));

            break;
        }
        case NewTicket:
        {
            ui->ticketDateTime->setDate(QDate::currentDate());
            ui->serviceForm->hide();
            ui->ticketForm->show();
            setWindowTitle(tr("Create new ticket"));

            ServiceList.GetMyServiceAddresses(myServices);
            for(std::multiset< std::pair< CScript, std::tuple<std::string, std::string, std::string> > >::const_iterator it = myServices.begin(); it!=myServices.end(); it++ )
            {
                if(get<2>(it->second) == "TicketSales" || get<2>(it->second) == "Ticketsales") {
                    ui->ticketService->addItem(QString::fromStdString(get<0>(it->second)));
                }
            }
            break;
        }
    }
}

EditServiceDialog::~EditServiceDialog()
{
    delete ui;
}

void EditServiceDialog::setModel(WalletModel *model)
{
    this->model = model;
    if(!model)
        return;
}

void EditServiceDialog::accept()
{
    if(!model)
        return;

     switch(mode)
     {
         case NewService:
         {
             CBitcoinAddress sAddress = CBitcoinAddress(ui->serviceAddress->text().toStdString());
             if (!sAddress.IsValid()) {
                 QMessageBox::warning(this, windowTitle(),
                                      tr("The entered address \"%1\" is not a valid Smileycoin address.").arg(ui->serviceAddress->text()),
                                      QMessageBox::Ok, QMessageBox::Ok);
                 return;
             } else {
                 // Get new service name and convert to hex
                 QString serviceName = ui->serviceName->text().toLatin1().toHex();
                 // Get new service address and convert to hex
                 QString serviceAddress = ui->serviceAddress->text().toLatin1().toHex();
                 // Get type of new service and convert to hex
                 QString rawServiceType = ui->serviceType->currentText().toLatin1().toHex();
                 std::vector<std::string> typeStr = splitString(rawServiceType.toStdString(), "20");
                 // Merge into one string if service type name consists of more than one word
                 QString serviceType = "";
                 if (typeStr.size() > 1) {
                     for (std::string::size_type i = 0; i < typeStr.size(); i++) {
                         serviceType += QString::fromStdString(typeStr.at(i));
                     }
                 } else {
                     serviceType = rawServiceType;
                 }

                 SendCoinsRecipient issuer;
                 // Send new service request transaction to official service address
                 //issuer.address = QString::fromStdString("B8dytMfspUhgMQUWGgdiR3QT8oUbNS9QVn");
                 issuer.address = QString::fromStdString("B9TRXJzgUJZZ5zPZbywtNfZHeu492WWRxc");

                 // Start with n = 10 (0.001) to get rid of spam
                 issuer.amount = 1000000000;

                 // Create op_return in the following form OP_RETURN = "new service serviceName serviceAddress serviceType"
                 issuer.data = QString::fromStdString("6e6577207365727669636520") + serviceName +
                               QString::fromStdString("20") + serviceAddress + QString::fromStdString("20") + serviceType;

                 QList <SendCoinsRecipient> recipients;
                 recipients.append(issuer);

                 // Format confirmation message
                 QStringList formatted;

                 // generate bold amount string
                 QString amount = "<b>" + BitcoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), issuer.amount);
                 amount.append("</b>");

                 // generate monospace address string
                 QString address2 = "<span style='font-family: monospace;'>" + issuer.address;
                 address2.append("</span>");

                 QString recipientElement = tr("%1 to %2").arg(amount, address2);

                 formatted.append(recipientElement);

                 WalletModelTransaction currentTransaction(recipients);
                 WalletModel::SendCoinsReturn prepareStatus;
                 if (model->getOptionsModel()->getCoinControlFeatures()) // coin control enabled
                     prepareStatus = model->prepareTransaction(currentTransaction, CoinControlDialog::coinControl);
                 else
                     prepareStatus = model->prepareTransaction(currentTransaction);

                 // process prepareStatus and on error generate message shown to user
                 processSendCoinsReturn(prepareStatus,
                                        BitcoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(),
                                                                     currentTransaction.getTransactionFee()));

                 qint64 txFee = currentTransaction.getTransactionFee();
                 QString questionString = tr("Are you sure you want to create a new service?");
                 questionString.append("<br /><br />%1");

                 if (txFee > 0) {
                     // append fee string if a fee is required
                     questionString.append("<hr /><span style='color:#aa0000;'>");
                     questionString.append(BitcoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), txFee));
                     questionString.append("</span> ");
                     questionString.append(tr("added as transaction fee"));
                 }

                 // add total amount in all subdivision units
                 questionString.append("<hr />");
                 qint64 totalAmount = currentTransaction.getTotalTransactionAmount() + txFee;
                 QStringList alternativeUnits;
                 foreach(BitcoinUnits::Unit u, BitcoinUnits::availableUnits())
                 {
                     if (u != model->getOptionsModel()->getDisplayUnit())
                         alternativeUnits.append(BitcoinUnits::formatWithUnit(u, totalAmount));
                 }
                 questionString.append(tr("Total Amount %1 (= %2)")
                                               .arg(BitcoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(),
                                                                                 totalAmount))
                                               .arg(alternativeUnits.join(" " + tr("or") + " ")));

                 QMessageBox::StandardButton retval = QMessageBox::question(this,
                                                                            tr("Confirm new service"),
                                                                            questionString.arg(formatted.join("<br />")),
                                                                            QMessageBox::Yes | QMessageBox::Cancel,
                                                                            QMessageBox::Cancel);

                 if (retval == QMessageBox::Yes) {
                     // now send the prepared transaction
                     WalletModel::SendCoinsReturn sendStatus = model->sendCoins(currentTransaction);
                     processSendCoinsReturn(sendStatus);

                     if (sendStatus.status == WalletModel::OK) {
                         QDialog::accept();
                         CoinControlDialog::coinControl->UnSelectAll();
                     }
                 }
             }
             break;
         }
         case DeleteService:
         {
             LogPrintStr(" deleteservice-editservicedialog-208 ");
             break;
         }
         case NewTicket:
         {
             CBitcoinAddress tAddress = CBitcoinAddress(ui->ticketAddress->text().toStdString());
             if (!tAddress.IsValid()) {
                 QMessageBox::warning(this, windowTitle(),
                                      tr("The entered address \"%1\" is not a valid Smileycoin address.").arg(ui->ticketAddress->text()),
                                      QMessageBox::Ok, QMessageBox::Ok);
                 return;
             } else {
                 QString rawTicketService = ui->ticketService->currentText();
                 QString rawTicketLoc = ui->ticketLocation->text().toLatin1().toHex();
                 QString rawTicketName = ui->ticketName->text().toLatin1().toHex();
                 QString ticketDateTime = ui->ticketDateTime->dateTime().toString("dd/MM/yyyyhh:mm").toLatin1().toHex();
                 QString ticketPrice = ui->ticketPrice->text().toLatin1().toHex();
                 QString ticketAddress = ui->ticketAddress->text().toLatin1().toHex();

                 std::vector<std::string> nameStr = splitString(rawTicketName.toStdString(), "20");
                 std::vector<std::string> locStr = splitString(rawTicketLoc.toStdString(), "20");

                 // Merge into one string if ticket name or ticket location consists of more than one word
                 QString ticketName = "";
                 if (nameStr.size() > 1) {
                     for (std::string::size_type i = 0; i < nameStr.size(); i++) {
                         ticketName += QString::fromStdString(nameStr.at(i));
                     }
                 } else {
                     ticketName = rawTicketName;
                 }

                 QString ticketLoc = "";
                 if (locStr.size() > 1) {
                     for (std::string::size_type i = 0; i < locStr.size(); i++) {
                         ticketLoc += QString::fromStdString(locStr.at(i));
                     }
                 } else {
                     ticketLoc = rawTicketLoc;
                 }


                 QString ticketServiceAddress = "";
                 SendCoinsRecipient issuer;
                 // Send new ticket to own service address
                 for(std::set< std::pair< CScript, std::tuple<std::string, std::string, std::string> > >::const_iterator it = myServices.begin(); it!=myServices.end(); it++ )
                 {
                     if(rawTicketService == QString::fromStdString(get<0>(it->second))) {
                         ticketServiceAddress = QString::fromStdString(get<1>(it->second));
                     }
                 }
                 issuer.address = ticketServiceAddress;
                 // Start with n = 1 to get rid of spam
                 issuer.amount = 100000000;

                 // Create op_return in the following form OP_RETURN = "new ticket ticketLoc ticketName ticketDate ticketTime ticketPrice ticketAddress"
                 issuer.data = QString::fromStdString("6e6577207469636b657420") +
                                  ticketLoc + QString::fromStdString("20") +
                                  ticketName + QString::fromStdString("20") +
                                  ticketDateTime + QString::fromStdString("20") +
                                  ticketPrice + QString::fromStdString("20") +
                                  ticketAddress;

                 QList <SendCoinsRecipient> recipients;
                 recipients.append(issuer);

                 // Format confirmation message
                 QStringList formatted;

                 // generate bold amount string
                 QString amount = "<b>" + BitcoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), issuer.amount);
                 amount.append("</b>");

                 // generate monospace address string
                 QString address2 = "<span style='font-family: monospace;'>" + issuer.address;
                 address2.append("</span>");

                 QString recipientElement = tr("%1 to %2").arg(amount, address2);

                 formatted.append(recipientElement);

                 WalletModelTransaction currentTransaction(recipients);
                 WalletModel::SendCoinsReturn prepareStatus;
                 if (model->getOptionsModel()->getCoinControlFeatures()) // coin control enabled
                     prepareStatus = model->prepareTransaction(currentTransaction, CoinControlDialog::coinControl);
                 else
                     prepareStatus = model->prepareTransaction(currentTransaction);

                 // process prepareStatus and on error generate message shown to user
                 processSendCoinsReturn(prepareStatus,
                                        BitcoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(),
                                                                     currentTransaction.getTransactionFee()));

                 qint64 txFee = currentTransaction.getTransactionFee();
                 QString questionString = tr("Are you sure you want to create a new ticket?");
                 questionString.append("<br /><br />%1");

                 if (txFee > 0) {
                     // append fee string if a fee is required
                     questionString.append("<hr /><span style='color:#aa0000;'>");
                     questionString.append(BitcoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), txFee));
                     questionString.append("</span> ");
                     questionString.append(tr("added as transaction fee"));
                 }

                 // add total amount in all subdivision units
                 questionString.append("<hr />");
                 qint64 totalAmount = currentTransaction.getTotalTransactionAmount() + txFee;
                 QStringList alternativeUnits;
                 foreach(BitcoinUnits::Unit u, BitcoinUnits::availableUnits())
                 {
                     if (u != model->getOptionsModel()->getDisplayUnit())
                         alternativeUnits.append(BitcoinUnits::formatWithUnit(u, totalAmount));
                 }
                 questionString.append(tr("Total Amount %1 (= %2)")
                                               .arg(BitcoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(),
                                                                                 totalAmount))
                                               .arg(alternativeUnits.join(" " + tr("or") + " ")));

                 QMessageBox::StandardButton retval = QMessageBox::question(this,
                                                                            tr("Confirm new ticket"),
                                                                            questionString.arg(formatted.join("<br />")),
                                                                            QMessageBox::Yes | QMessageBox::Cancel,
                                                                            QMessageBox::Cancel);

                 if (retval == QMessageBox::Yes) {
                     // now send the prepared transaction
                     WalletModel::SendCoinsReturn sendStatus = model->sendCoins(currentTransaction);
                     processSendCoinsReturn(sendStatus);

                     if (sendStatus.status == WalletModel::OK) {
                         QDialog::accept();
                         CoinControlDialog::coinControl->UnSelectAll();
                     } else {
                         QDialog::reject();
                     }
                 }
             }
             break;
         }
     }
}

void EditServiceDialog::processSendCoinsReturn(const WalletModel::SendCoinsReturn &sendCoinsReturn, const QString &msgArg)
{
    QPair<QString, CClientUIInterface::MessageBoxFlags> msgParams;
    // Default to a warning message, override if error message is needed
    msgParams.second = CClientUIInterface::MSG_WARNING;

    // This comment is specific to SendCoinsDialog usage of WalletModel::SendCoinsReturn.
    // WalletModel::TransactionCommitFailed is used only in WalletModel::sendCoins()
    // all others are used only in WalletModel::prepareTransaction()
    switch(sendCoinsReturn.status)
    {
        case WalletModel::InvalidAddress:
            msgParams.first = tr("The entered address \"%1\" is not a valid Smileycoin address.");
            break;
        case WalletModel::AmountWithFeeExceedsBalance:
            msgParams.first = tr("The total exceeds your balance when the %1 transaction fee is included.").arg(msgArg);
            break;
        case WalletModel::TransactionCreationFailed:
            msgParams.first = tr("Transaction creation failed!");
            msgParams.second = CClientUIInterface::MSG_ERROR;
            break;
        case WalletModel::TransactionCommitFailed:
            msgParams.first = tr("The transaction was rejected! This might happen if some of the coins in your wallet were already spent, such as if you used a copy of wallet.dat and coins were spent in the copy but not marked as spent here.");
            msgParams.second = CClientUIInterface::MSG_ERROR;
            break;
            // included to prevent a compiler warning.
        case WalletModel::OK:
        default:
            return;
    }
}