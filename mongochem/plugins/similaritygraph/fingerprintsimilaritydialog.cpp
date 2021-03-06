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

#include "mongodatabase.h"
#include "chemkit.h"

#include "fingerprintsimilaritydialog.h"
#include "ui_fingerprintsimilaritydialog.h"

#include <chemkit/fingerprint.h>

using namespace chemkit;

FingerprintSimilarityDialog::FingerprintSimilarityDialog(QWidget *parent_)
  : MongoChem::AbstractClusteringWidget(parent_),
    ui(new Ui::FingerprintSimilarityDialog)
{
  ui->setupUi(this);

  m_graphWidget = new SimilarityGraphWidget(this);
  ui->similaritySlider->setValue(45);
  m_graphWidget->setSimilarityThreshold(
    static_cast<float>(ui->similaritySlider->value()) / 100.f);
  ui->mainLayout->addWidget(m_graphWidget);

  connect(ui->similaritySlider, SIGNAL(sliderPressed()),
          this, SLOT(similaritySliderPressed()));
  connect(ui->similaritySlider, SIGNAL(valueChanged(int)),
          this, SLOT(similarityValueChanged(int)));
  connect(ui->similaritySlider, SIGNAL(valueChanged(int)),
          ui->similaritySpinBox, SLOT(setValue(int)));
  connect(ui->similaritySpinBox, SIGNAL(valueChanged(int)),
          ui->similaritySlider, SLOT(setValue(int)));
  connect(ui->similaritySlider, SIGNAL(sliderReleased()),
          this, SLOT(similaritySliderReleased()));
  connect(ui->fingerprintComboBox, SIGNAL(currentIndexChanged(QString)),
          this, SLOT(setFingerprint(QString)));
  connect(m_graphWidget, SIGNAL(vertexDoubleClicked(vtkIdType)),
          this, SLOT(vertexDoubleClicked(vtkIdType)));
}

FingerprintSimilarityDialog::~FingerprintSimilarityDialog()
{
  delete ui;
}

void FingerprintSimilarityDialog::setMolecules(const std::vector<MongoChem::MoleculeRef> &molecules)
{
  m_molecules = molecules;

  setFingerprint(ui->fingerprintComboBox->currentText());
}

void FingerprintSimilarityDialog::setFingerprint(const QString &name)
{
  // calculate fingerprints
  boost::scoped_ptr<Fingerprint> fingerprint_(Fingerprint::create(name.toStdString()));

  std::vector<Bitset> fingerprints;
  for (size_t i = 0; i < m_molecules.size(); ++i) {
    const MongoChem::MoleculeRef &ref = m_molecules[i];
    boost::shared_ptr<const chemkit::Molecule> molecule =
        MongoChem::ChemKit::createMolecule(ref);

    if (molecule)
      fingerprints.push_back(fingerprint_->value(molecule.get()));
    else
      fingerprints.push_back(chemkit::Bitset());
  }

  // calculate similarity matrix
  Eigen::MatrixXf similarityMatrix;
  similarityMatrix.resize(m_molecules.size(), m_molecules.size());

  for (size_t i = 0; i < fingerprints.size(); ++i) {
    for (size_t j = i + 1; j < fingerprints.size(); ++j) {
      float similarity =
        static_cast<float>(Fingerprint::tanimotoCoefficient(fingerprints[i],
                                                            fingerprints[j]));

      similarityMatrix(i, j) = similarity;
      similarityMatrix(j, i) = similarity;
    }
  }

  // set similarity matrix
  m_graphWidget->setSimilarityMatrix(similarityMatrix);
}

QString FingerprintSimilarityDialog::fingerprint() const
{
  return ui->fingerprintComboBox->currentText();
}

void FingerprintSimilarityDialog::similaritySliderPressed()
{
  m_graphWidget->setLayoutPaused(true);
}

void FingerprintSimilarityDialog::similaritySliderReleased()
{
  m_graphWidget->setLayoutPaused(false);
}

void FingerprintSimilarityDialog::similarityValueChanged(int value)
{
  m_graphWidget->setSimilarityThreshold(static_cast<float>(value) / 100.f);
}

void FingerprintSimilarityDialog::vertexDoubleClicked(vtkIdType id)
{
  emit moleculeDoubleClicked(m_molecules[id]);
}
