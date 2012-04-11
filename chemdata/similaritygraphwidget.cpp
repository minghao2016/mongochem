/******************************************************************************

  This source file is part of the ChemData project.

  Copyright 2012 Kitware, Inc.

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

******************************************************************************/

#include "similaritygraphwidget.h"

#include <QtGui>

#include <vtkNew.h>
#include <QVTKWidget.h>
#include <vtkViewTheme.h>
#include <vtkFloatArray.h>
#include <vtkIdTypeArray.h>
#include <vtkGraphLayoutView.h>
#include <vtkDataSetAttributes.h>
#include <vtkColorTransferFunction.h>
#include <vtkMutableUndirectedGraph.h>
#include <vtkForceDirectedLayoutStrategy.h>

class SimilarityGraphWidgetPrivate
{
public:
  bool layoutPaused;
  float similarityThreshold;
  Eigen::MatrixXf similarityMatrix;
  QVTKWidget *vtkWidget;
  vtkNew<vtkMutableUndirectedGraph> graph;
  vtkNew<vtkForceDirectedLayoutStrategy> layoutStrategy;
  QMutex layoutGraphMutex;
  vtkNew<vtkGraphLayoutView> graphView;
};

SimilarityGraphWidget::SimilarityGraphWidget(QWidget *parent)
  : QWidget(parent),
    d(new SimilarityGraphWidgetPrivate)
{
  d->layoutPaused = false;

  // setup graph and layout strategy
  d->layoutStrategy->SetGraph(d->graph.GetPointer());
  d->layoutStrategy->SetIterationsPerLayout(10);
  d->layoutStrategy->SetMaxNumberOfIterations(200);

  // setup layout and qvtk widget
  QVBoxLayout *layout = new QVBoxLayout;

  d->vtkWidget = new QVTKWidget(this);
  d->graphView->SetInteractor(d->vtkWidget->GetInteractor());
  d->vtkWidget->SetRenderWindow(d->graphView->GetRenderWindow());
  d->graphView->SetLayoutStrategyToPassThrough();
  d->graphView->SetRepresentationFromInput(d->graph.GetPointer());
  d->graphView->SetEdgeColorArrayName("weights");
  d->graphView->SetColorEdges(true);
  layout->addWidget(d->vtkWidget);

  setLayout(layout);

  // setup edge color function
  vtkSmartPointer<vtkColorTransferFunction> colorTransferFunction =
    vtkSmartPointer<vtkColorTransferFunction>::New();
  colorTransferFunction->AddRGBPoint(0.0, 0.0, 0.0, 1.0);
  colorTransferFunction->AddRGBPoint(0.15, 0.0, 1.0, 1.0);
  colorTransferFunction->AddRGBPoint(0.33, 1.0, 1.0, 0.0);
  colorTransferFunction->AddRGBPoint(0.66, 1.0, 0.0, 0.0);
  colorTransferFunction->AddRGBPoint(1.0, 1.0, 0.0, 0.0);
  colorTransferFunction->Build();
  vtkViewTheme *theme = vtkViewTheme::New();
  theme->SetCellLookupTable(colorTransferFunction);
  d->graphView->ApplyViewTheme(theme);
  theme->Delete();

  connect(this, SIGNAL(readyToRender()), SLOT(renderGraph()), Qt::QueuedConnection);
}

SimilarityGraphWidget::~SimilarityGraphWidget()
{
  delete d;
}

void SimilarityGraphWidget::setSimilarityMatrix(const Eigen::MatrixXf &matrix)
{
  d->similarityMatrix = matrix;

  d->graph->SetNumberOfVertices(matrix.rows());

  // update graph
  setSimilarityThreshold(d->similarityThreshold);
}

const Eigen::MatrixXf& SimilarityGraphWidget::similarityMatrix() const
{
  return d->similarityMatrix;
}

void SimilarityGraphWidget::setSimilarityThreshold(float value)
{
  d->similarityThreshold = value;

  d->layoutGraphMutex.lock();

  // clear all edges from graph
  vtkIdTypeArray *edges = vtkIdTypeArray::New();
  for(size_t i = 0; i < d->graph->GetNumberOfEdges(); i++){
    edges->InsertNextValue(i);
  }
  d->graph->RemoveEdges(edges);
  edges->Delete();

  vtkFloatArray *weights = vtkFloatArray::New();
  weights->SetName("weights");

  // add edges that exceed similarity threshold
  for (size_t i = 0; i < d->similarityMatrix.rows(); i++) {
    for (size_t j = i + 1; j < d->similarityMatrix.cols(); j++) {
      float similarity = d->similarityMatrix(i, j);

      if (similarity > d->similarityThreshold) {
        d->graph->AddEdge(i, j);
        weights->InsertNextValue(similarity);
      }
    }
  }

  d->graph->GetEdgeData()->AddArray(weights);
  weights->Delete();

  // setup layout strategy
  d->layoutStrategy->SetGraph(d->graph.GetPointer());
  d->layoutStrategy->Initialize();
  d->layoutGraphMutex.unlock();

  // render graph
  renderGraph();
}

float SimilarityGraphWidget::similarityThreshold() const
{
  return d->similarityThreshold;
}

void SimilarityGraphWidget::setLayoutPaused(bool paused)
{
  if (paused == d->layoutPaused) {
    // nothing to change
    return;
  }

  bool wasPaused = d->layoutPaused;

  d->layoutPaused = paused;

  if (wasPaused) {
    // render graph
    renderGraph();
  }
}

bool SimilarityGraphWidget::isLayoutPaused() const
{
  return d->layoutPaused;
}

void SimilarityGraphWidget::updateLayout()
{
  // update layout
  d->layoutGraphMutex.lock();
  d->layoutStrategy->Layout();
  d->layoutGraphMutex.unlock();

  // render graph on next event loop
  emit readyToRender();
}

void SimilarityGraphWidget::renderGraph()
{
  // render graph
  d->vtkWidget->update();

  if(!d->layoutPaused && !d->layoutStrategy->IsLayoutComplete())
    QtConcurrent::run(this, &SimilarityGraphWidget::updateLayout);
}