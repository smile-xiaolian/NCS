#include <QApplication>
#include <QWidget>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>
#include <QDebug>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarCategoryAxis>
#include "core/service/PlatformService.h"
class Admin:public QWidget{QStackedWidget*p=new QStackedWidget;QTableWidget *chargers=new QTableWidget,*users=new QTableWidget;QLabel*metrics=new QLabel;
void table(QTableWidget*w,QStringList h){w->setColumnCount(h.size());w->setHorizontalHeaderLabels(h);w->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);w->setSelectionBehavior(QAbstractItemView::SelectRows);w->setEditTriggers(QAbstractItemView::NoEditTriggers);}void fillChargers(){auto a=PlatformService::chargers();chargers->setRowCount(a.size());for(int i=0;i<a.size();i++){auto m=a[i].toMap();QStringList v={m["code"].toString(),m["station_name"].toString(),m["type"].toString(),QString::number(m["power"].toDouble()),m["status"].toInt()==0?"空闲":m["status"].toInt()==1?"使用中":"故障",QString::number(m["total_count"].toInt())};for(int j=0;j<v.size();j++)chargers->setItem(i,j,new QTableWidgetItem(v[j]));chargers->item(i,0)->setData(Qt::UserRole,m["id"]);}}void fillUsers(){auto a=PlatformService::users();users->setRowCount(a.size());for(int i=0;i<a.size();i++){auto m=a[i].toMap();QStringList v={QString::number(m["id"].toInt()),m["phone"].toString(),m["nickname"].toString(),QString::number(m["balance"].toDouble(),'f',2),m["status"].toInt()?"正常":"冻结",m["created_at"].toString()};for(int j=0;j<v.size();j++)users->setItem(i,j,new QTableWidgetItem(v[j]));}}void stats(){auto m=PlatformService::metrics();metrics->setText(QString("<h2>运营总览</h2>总充电订单：%1　总营收：¥ %2　在线电桩：%3　注册用户：%4").arg(m["orders"].toInt()).arg(m["revenue"].toDouble(),0,'f',2).arg(m["online"].toInt()).arg(m["users"].toInt()));}
public:Admin(){setWindowTitle("NCS 运营管理后台");resize(1280,800);auto root=new QVBoxLayout(this);root->addWidget(p);auto login=new QWidget;auto ll=new QVBoxLayout(login);auto a=new QLineEdit,b=new QLineEdit;a->setPlaceholderText("账号");b->setPlaceholderText("密码");b->setEchoMode(QLineEdit::Password);auto in=new QPushButton("登录（默认 admin / 123456）");ll->addWidget(new QLabel("<h1>NCS 管理端</h1>"));ll->addWidget(a);ll->addWidget(b);ll->addWidget(in);ll->addStretch();p->addWidget(login);
auto main=new QWidget;auto ml=new QHBoxLayout(main);auto nav=new QVBoxLayout;auto *overview=new QPushButton("营收分析"),*cp=new QPushButton("充电桩管理"),*up=new QPushButton("用户管理"),*refresh=new QPushButton("刷新全部");for(auto*x:{overview,cp,up,refresh})nav->addWidget(x);nav->addStretch();ml->addLayout(nav);auto work=new QStackedWidget;ml->addWidget(work,1);auto dash=new QWidget;auto dl=new QVBoxLayout(dash);dl->addWidget(metrics);auto chart=new QChartView;dl->addWidget(chart);work->addWidget(dash);auto cv=new QWidget;auto cl=new QVBoxLayout(cv);table(chargers,{"编号","所属电站","类型","功率","状态","累计次数"});cl->addWidget(chargers);auto fault=new QPushButton("标记故障 / 恢复正常");cl->addWidget(fault);work->addWidget(cv);auto uv=new QWidget;auto ul=new QVBoxLayout(uv);table(users,{"ID","手机号","昵称","余额","状态","注册时间"});ul->addWidget(users);auto freeze=new QPushButton("冻结 / 解冻选中用户");ul->addWidget(freeze);work->addWidget(uv);p->addWidget(main);
auto render=[this,chart]{stats();fillChargers();fillUsers();auto series=new QLineSeries;int i=0;for(auto&z:PlatformService::revenueDays())series->append(i++,z.toMap()["revenue"].toDouble());auto c=new QChart;c->addSeries(series);c->createDefaultAxes();c->setTitle("近 30 日营收趋势（完成订单）");chart->setChart(c);};connect(in,&QPushButton::clicked,this,[this,a,b,render]{if(!PlatformService::adminLogin(a->text(),b->text())){QMessageBox::warning(this,"NCS","账号或密码错误");return;}render();p->setCurrentIndex(1);});connect(overview,&QPushButton::clicked,this,[work,render]{work->setCurrentIndex(0);render();});connect(cp,&QPushButton::clicked,this,[this,work]{work->setCurrentIndex(1);fillChargers();});connect(up,&QPushButton::clicked,this,[this,work]{work->setCurrentIndex(2);fillUsers();});connect(refresh,&QPushButton::clicked,this,render);connect(fault,&QPushButton::clicked,this,[this]{auto x=chargers->currentItem();if(!x)return;int id=chargers->item(x->row(),0)->data(Qt::UserRole).toInt();int s=chargers->item(x->row(),4)->text()=="故障"?0:2;PlatformService::setChargerStatus(id,s);fillChargers();});connect(freeze,&QPushButton::clicked,this,[this]{auto x=users->currentItem();if(!x)return;int id=users->item(x->row(),0)->text().toInt();int s=users->item(x->row(),4)->text()=="正常"?0:1;PlatformService::setUserStatus(id,s);fillUsers();});}}
;
QString findPython(){
    const QStringList candidates={"python3","python"};
    for(const auto&c:candidates){const auto p=QStandardPaths::findExecutable(c);if(!p.isEmpty())return p;}
    return {};
}

void startReportProcess(QProcess& process){
    const QString python=findPython();
    const QString script=QCoreApplication::applicationDirPath()+"/generate_region_report.py";
    if(python.isEmpty()){qWarning()<<"report generator skipped: python not found";return;}
    if(!QFileInfo::exists(script)){qWarning()<<"report generator skipped: script not found"<<script;return;}

    const QString db=PlatformService::databasePath();
    const QString projectRoot=QDir::cleanPath(QCoreApplication::applicationDirPath()+"/../..");
    const QString out=projectRoot+"/charge_report.json";
    process.setProcessChannelMode(QProcess::ForwardedChannels);
    process.start(python,{script,"--db",db,"--out",out,"--interval","30"});
}

void stopReportProcess(QProcess& process){
    if(process.state()==QProcess::NotRunning)return;
    process.terminate();
    if(!process.waitForFinished(3000)){
        process.kill();
        process.waitForFinished(1000);
    }
}

int main(int c,char**v){
    QApplication a(c,v);
    QString e;
    if(!PlatformService::initialize(&e)){QMessageBox::critical(nullptr,"NCS",e);return 1;}

    QProcess reportProcess;
    QObject::connect(&reportProcess,&QProcess::errorOccurred,[](QProcess::ProcessError err){
        if(err!=QProcess::Crashed)qWarning()<<"report generator error:"<<err;
    });
    startReportProcess(reportProcess);
    QObject::connect(&a,&QCoreApplication::aboutToQuit,[&reportProcess]{stopReportProcess(reportProcess);});

    Admin w;
    w.show();
    const int ret=a.exec();
    stopReportProcess(reportProcess);
    return ret;
}
