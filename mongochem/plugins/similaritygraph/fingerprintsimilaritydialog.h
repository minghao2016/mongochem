/******************************************************************************

  This source file is part of the MongoChem project.

  Copyright 2012 Kitware, Inc.

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

******************************************************************************/

#ifndef FINGERPRINTSIMILARITYDIALOG_H
#define FINGERPRINTSIMILARITYDIALOG_H

#include <mongochem/gui/abstractclusteringwidget.h>

#include "similaritygraphwidget.h"

namespace Ui {
class FingerprintSimilarityDialog;
}

class FingerprintSimilarityDialog : public MongoChem::AbstractClusteringWidget
{
  Q_OBJECT

public:
  /** Creates a new structure similarity dialog with @p parent. */
  explicit FingerprintSimilarityDialog(QWidget *parent = 0);

  /** Destroys the structure similarity dialog. */
  ~FingerprintSimilarityDialog();

  /** Sets the molecules to display in the graph. */
  void setMolecules(const std::vector<MongoChem::MoleculeRef> &molecules);

public slots:
  /** Sets the fingerprint to use for calculating similarity to @p name. */
  void setFingerprint(const QString &name);

  /** Returns the name of the fingerprint being used to calculate similarity. */
  QString fingerprint() const;

private slots:
  void similaritySliderPressed();
  void similaritySliderReleased();
  void similarityValueChanged(int value);
  void vertexDoubleClicked(vtkIdType id);

private:
  Ui::FingerprintSimilarityDialog *ui;
  SimilarityGraphWidget *m_graphWidget;
  std::vector<MongoChem::MoleculeRef> m_molecules;
};

#endif // FINGERPRINTSIMILARITYDIALOG_H
