/******************************************************************************

  This source file is part of the ChemData project.

  Copyright 2011-2012 Kitware, Inc.

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

******************************************************************************/

#include "graphdialog.h"
#include "ui_graphdialog.h"

#include <mongo/client/dbclient.h>

#include <QtCore/QSettings>

#include <QVTKInteractor.h>
#include <vtkContextView.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkContextScene.h>
#include <vtkChartXY.h>
#include <vtkAxis.h>
#include <vtkFloatArray.h>
#include <vtkStringArray.h>
#include <vtkPlot.h>
#include <QVTKWidget.h>
#include <vtkTable.h>
#include <vtkContextView.h>
#include <vtkChartXY.h>

#include "diagramtooltipitem.h"

using namespace mongo;

GraphDialog::GraphDialog(QWidget *parent)
  : QDialog(parent),
    ui(new Ui::GraphDialog)
{
  ui->setupUi(this);

  connect(ui->showButton, SIGNAL(clicked()), SLOT(showClicked()));

  vtkFloatArray *xArray = vtkFloatArray::New();
  xArray->SetName("X");

  vtkFloatArray *yArray = vtkFloatArray::New();
  yArray->SetName("Y");

  m_table->AddColumn(xArray);
  m_table->AddColumn(yArray);

  xArray->Delete();
  yArray->Delete();

  m_vtkWidget = new QVTKWidget(this);

  m_chartView->SetInteractor(m_vtkWidget->GetInteractor());
  m_vtkWidget->SetRenderWindow(m_chartView->GetRenderWindow());

  m_chartView->GetScene()->AddItem(m_chart.GetPointer());
  vtkPlot *scatter = m_chart->AddPlot(vtkChart::POINTS);
  scatter->SetInputData(m_table.GetPointer(), "X", "Y");
  scatter->SetColor(0, 0, 1.0);

  // create labels array
  vtkStringArray *nameArray = vtkStringArray::New();
  nameArray->SetName("name");
  scatter->SetIndexedLabels(nameArray);
  scatter->SetTooltipLabelFormat("%i");
  nameArray->Delete();

  m_chart->SetRenderEmpty(true);
  m_chart->SetAutoAxes(false);
  m_chart->SetDrawAxesAtOrigin(true);

  // set tooltip item
  DiagramTooltipItem *tooltip = DiagramTooltipItem::New();
  m_chart->SetTooltip(tooltip);
  tooltip->Delete();

  QVBoxLayout *graphLayout = new QVBoxLayout;
  graphLayout->addWidget(m_vtkWidget);
  ui->graphFrame->setLayout(graphLayout);
}

GraphDialog::~GraphDialog()
{
  delete ui;
}

void GraphDialog::showClicked()
{
  QString xName = ui->xComboBox->currentText().toLower();
  QString yName = ui->yComboBox->currentText().toLower();

  QSettings settings;
  std::string host = settings.value("hostname").toString().toStdString();
  DBClientConnection db;
  try {
    db.connect(host);
  }
  catch (DBException &e) {
    std::cerr << "Failed to connect to MongoDB: " << e.what() << std::endl;
    return;
  }

  vtkFloatArray *xArray = vtkFloatArray::SafeDownCast(m_table->GetColumnByName("X"));
  vtkFloatArray *yArray = vtkFloatArray::SafeDownCast(m_table->GetColumnByName("Y"));
  vtkStringArray *nameArray = m_chart->GetPlot(0)->GetIndexedLabels();

  // clear current data
  xArray->SetNumberOfValues(0);
  yArray->SetNumberOfValues(0);
  nameArray->SetNumberOfValues(0);

  std::string collection = settings.value("collection").toString().toStdString();
  std::string moleculesCollection = collection + ".molecules";

  // query for x data
  auto_ptr<DBClientCursor> cursor = db.query(moleculesCollection);

  while(cursor->more()){
    BSONObj obj = cursor->next();

    // get values
    double xValue = obj.getFieldDotted("descriptors." + xName.toStdString()).numberDouble();
    double yValue = obj.getFieldDotted("descriptors." + yName.toStdString()).numberDouble();

    // insert into table
    xArray->InsertNextValue(xValue);
    yArray->InsertNextValue(yValue);

    // add name to tooltip array
    std::string name = obj.getField("name").str();
    nameArray->InsertNextValue(name);
  }

  // refesh view
  m_table->Modified();
  m_chart->RecalculateBounds();
  m_vtkWidget->update();
}
