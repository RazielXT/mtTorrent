/********************************************************************************
** Form generated from reading UI file 'mtTorrentGui.ui'
**
** Created by: Qt User Interface Compiler version 5.8.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MTTORRENTGUI_H
#define UI_MTTORRENTGUI_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_mtTorrentGuiClass
{
public:
    QWidget *centralWidget;
    QGridLayout *gridLayout;
    QFrame *frame;
    QPushButton *pushButton;
    QFrame *frame_2;
    QGridLayout *gridLayout_2;
    QSplitter *splitter_2;
    QFrame *frame_3;
    QGridLayout *gridLayout_3;
    QSplitter *splitter;
    QListWidget *listWidget;
    QTreeWidget *treeWidget;
    QTabWidget *tabWidget;
    QWidget *tab;
    QWidget *tab_2;
    QGridLayout *gridLayout_4;
    QListWidget *listWidget_2;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *mtTorrentGuiClass)
    {
        if (mtTorrentGuiClass->objectName().isEmpty())
            mtTorrentGuiClass->setObjectName(QStringLiteral("mtTorrentGuiClass"));
        mtTorrentGuiClass->resize(983, 684);
        centralWidget = new QWidget(mtTorrentGuiClass);
        centralWidget->setObjectName(QStringLiteral("centralWidget"));
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(centralWidget->sizePolicy().hasHeightForWidth());
        centralWidget->setSizePolicy(sizePolicy);
        gridLayout = new QGridLayout(centralWidget);
        gridLayout->setSpacing(6);
        gridLayout->setContentsMargins(11, 11, 11, 11);
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        frame = new QFrame(centralWidget);
        frame->setObjectName(QStringLiteral("frame"));
        QSizePolicy sizePolicy1(QSizePolicy::Expanding, QSizePolicy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(frame->sizePolicy().hasHeightForWidth());
        frame->setSizePolicy(sizePolicy1);
        frame->setMinimumSize(QSize(0, 50));
        frame->setAutoFillBackground(true);
        frame->setFrameShape(QFrame::StyledPanel);
        frame->setFrameShadow(QFrame::Raised);
        pushButton = new QPushButton(frame);
        pushButton->setObjectName(QStringLiteral("pushButton"));
        pushButton->setGeometry(QRect(40, 10, 93, 28));

        gridLayout->addWidget(frame, 0, 0, 1, 1);

        frame_2 = new QFrame(centralWidget);
        frame_2->setObjectName(QStringLiteral("frame_2"));
        sizePolicy.setHeightForWidth(frame_2->sizePolicy().hasHeightForWidth());
        frame_2->setSizePolicy(sizePolicy);
        frame_2->setAutoFillBackground(true);
        frame_2->setFrameShape(QFrame::StyledPanel);
        frame_2->setFrameShadow(QFrame::Raised);
        gridLayout_2 = new QGridLayout(frame_2);
        gridLayout_2->setSpacing(6);
        gridLayout_2->setContentsMargins(11, 11, 11, 11);
        gridLayout_2->setObjectName(QStringLiteral("gridLayout_2"));
        splitter_2 = new QSplitter(frame_2);
        splitter_2->setObjectName(QStringLiteral("splitter_2"));
        splitter_2->setOrientation(Qt::Vertical);
        frame_3 = new QFrame(splitter_2);
        frame_3->setObjectName(QStringLiteral("frame_3"));
        QSizePolicy sizePolicy2(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(frame_3->sizePolicy().hasHeightForWidth());
        frame_3->setSizePolicy(sizePolicy2);
        frame_3->setAutoFillBackground(true);
        frame_3->setFrameShape(QFrame::StyledPanel);
        frame_3->setFrameShadow(QFrame::Raised);
        gridLayout_3 = new QGridLayout(frame_3);
        gridLayout_3->setSpacing(6);
        gridLayout_3->setContentsMargins(11, 11, 11, 11);
        gridLayout_3->setObjectName(QStringLiteral("gridLayout_3"));
        splitter = new QSplitter(frame_3);
        splitter->setObjectName(QStringLiteral("splitter"));
        sizePolicy2.setHeightForWidth(splitter->sizePolicy().hasHeightForWidth());
        splitter->setSizePolicy(sizePolicy2);
        splitter->setAutoFillBackground(true);
        splitter->setOrientation(Qt::Horizontal);
        listWidget = new QListWidget(splitter);
        listWidget->setObjectName(QStringLiteral("listWidget"));
        sizePolicy2.setHeightForWidth(listWidget->sizePolicy().hasHeightForWidth());
        listWidget->setSizePolicy(sizePolicy2);
        splitter->addWidget(listWidget);
        treeWidget = new QTreeWidget(splitter);
        QTreeWidgetItem *__qtreewidgetitem = new QTreeWidgetItem();
        __qtreewidgetitem->setText(0, QStringLiteral("1"));
        treeWidget->setHeaderItem(__qtreewidgetitem);
        treeWidget->setObjectName(QStringLiteral("treeWidget"));
        sizePolicy2.setHeightForWidth(treeWidget->sizePolicy().hasHeightForWidth());
        treeWidget->setSizePolicy(sizePolicy2);
        treeWidget->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
        splitter->addWidget(treeWidget);

        gridLayout_3->addWidget(splitter, 0, 0, 1, 1);

        splitter_2->addWidget(frame_3);
        tabWidget = new QTabWidget(splitter_2);
        tabWidget->setObjectName(QStringLiteral("tabWidget"));
        sizePolicy2.setHeightForWidth(tabWidget->sizePolicy().hasHeightForWidth());
        tabWidget->setSizePolicy(sizePolicy2);
        tabWidget->setAutoFillBackground(true);
        tab = new QWidget();
        tab->setObjectName(QStringLiteral("tab"));
        tab->setAutoFillBackground(true);
        tabWidget->addTab(tab, QString());
        tab_2 = new QWidget();
        tab_2->setObjectName(QStringLiteral("tab_2"));
        tab_2->setAutoFillBackground(true);
        gridLayout_4 = new QGridLayout(tab_2);
        gridLayout_4->setSpacing(6);
        gridLayout_4->setContentsMargins(11, 11, 11, 11);
        gridLayout_4->setObjectName(QStringLiteral("gridLayout_4"));
        listWidget_2 = new QListWidget(tab_2);
        listWidget_2->setObjectName(QStringLiteral("listWidget_2"));

        gridLayout_4->addWidget(listWidget_2, 0, 0, 1, 1);

        tabWidget->addTab(tab_2, QString());
        splitter_2->addWidget(tabWidget);

        gridLayout_2->addWidget(splitter_2, 0, 0, 1, 1);


        gridLayout->addWidget(frame_2, 1, 0, 1, 1);

        mtTorrentGuiClass->setCentralWidget(centralWidget);
        statusBar = new QStatusBar(mtTorrentGuiClass);
        statusBar->setObjectName(QStringLiteral("statusBar"));
        mtTorrentGuiClass->setStatusBar(statusBar);

        retranslateUi(mtTorrentGuiClass);
        QObject::connect(pushButton, SIGNAL(clicked()), mtTorrentGuiClass, SLOT(addButtonPressed()));

        tabWidget->setCurrentIndex(1);


        QMetaObject::connectSlotsByName(mtTorrentGuiClass);
    } // setupUi

    void retranslateUi(QMainWindow *mtTorrentGuiClass)
    {
        mtTorrentGuiClass->setWindowTitle(QApplication::translate("mtTorrentGuiClass", "mtTorrentGui", Q_NULLPTR));
        pushButton->setText(QApplication::translate("mtTorrentGuiClass", "Add", Q_NULLPTR));
        tabWidget->setTabText(tabWidget->indexOf(tab), QApplication::translate("mtTorrentGuiClass", "Tab 1", Q_NULLPTR));
        tabWidget->setTabText(tabWidget->indexOf(tab_2), QApplication::translate("mtTorrentGuiClass", "Tab 2", Q_NULLPTR));
    } // retranslateUi

};

namespace Ui {
    class mtTorrentGuiClass: public Ui_mtTorrentGuiClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MTTORRENTGUI_H
