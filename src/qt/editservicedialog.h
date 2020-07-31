//
// Created by Lenovo on 6/15/2020.
//

#ifndef SMILEYCOIN_EDITSERVICEDIALOG_H
#define SMILEYCOIN_EDITSERVICEDIALOG_H

#include "walletmodel.h"
#include "guiutil.h"

#include <QDialog>
#include <QWidget>
#include <QGroupBox>
#include <QFormLayout>
#include <QComboBox>
#include <QFrame>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QDateTimeEdit>
#include <QDateTime>
#include <QDate>
#include <QTabWidget>
#include <QTableView>
#include <QScrollBar>
#include <QTableWidget>
#include <QDesktopWidget>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QApplication>
#include <QMessageBox>
#include <QTextTableFormat>
#include <QDebug>
#include <QStringList>

class WalletModel;
class OptionsModel;
class SendCoinsDialog;
class QValidatedLineEdit;

namespace Ui {
    class EditServiceDialog;
}

/** Dialog for creating a new service.
 */
class EditServiceDialog : public QDialog
{
    Q_OBJECT

public:
    enum Mode {
        NewService,
        DeleteService,
        NewTicket
    };

    explicit EditServiceDialog(Mode mode, QWidget *parent);
    ~EditServiceDialog();

    void setModel(WalletModel *model);

public slots:
    void accept();

private:
    Ui::EditServiceDialog *ui;
    Mode mode;
    WalletModel *model;
    std::multiset<std::pair< CScript, std::tuple<std::string, std::string, std::string>>> myServices;
    void processSendCoinsReturn(const WalletModel::SendCoinsReturn &sendCoinsReturn, const QString &msgArg = QString());
};

#endif //SMILEYCOIN_EDITSERVICEDIALOG_H
