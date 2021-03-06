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

#ifndef SERVERSETTINGSDIALOG_H
#define SERVERSETTINGSDIALOG_H

#include <QDialog>

namespace Ui {
class ServerSettingsDialog;
}

namespace MongoChem {

class ServerSettingsDialog : public QDialog
{
  Q_OBJECT

public:
  explicit ServerSettingsDialog(QWidget *parent = 0);
  ~ServerSettingsDialog();

  QString host() const;
  QString port() const;
  QString collection() const;
  QString userName() const;

private:
  Ui::ServerSettingsDialog *ui;
};

} // end MongoChem namespace

#endif // SERVERSETTINGSDIALOG_H
