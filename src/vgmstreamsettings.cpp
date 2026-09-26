/*
 * Fooyin
 * Copyright © 2026, Luke Taylor <luket@pm.me>
 *
 * Fooyin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Fooyin is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Fooyin.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "vgmstreamsettings.h"

#include "vgmstreamdefs.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>

using namespace Qt::StringLiterals;

namespace Fooyin::VGMStream {
VGMStreamSettings::VGMStreamSettings(QWidget* parent)
    : QDialog{parent}
    , m_loopCount{new QSpinBox(this)}
    , m_fadeLength{new QSpinBox(this)}
    , m_generateTitles{new QCheckBox(tr("Generate titles from filenames and stream names"), this)}
    , m_commonExtensions{new QCheckBox(tr("Enable common extensions"), this)}
{
    setWindowTitle(tr("VGMStream Settings"));

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    QObject::connect(buttons, &QDialogButtonBox::accepted, this, &VGMStreamSettings::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, this, &VGMStreamSettings::reject);

    auto* lengthGroup  = new QGroupBox(tr("Length"), this);
    auto* lengthLayout = new QGridLayout(lengthGroup);

    m_loopCount->setRange(1, 10);

    m_fadeLength->setRange(0, 10000);
    m_fadeLength->setSingleStep(500);
    m_fadeLength->setSuffix(u" ms"_s);

    lengthLayout->addWidget(new QLabel(tr("Loop count") + u":"_s, this), 0, 0);
    lengthLayout->addWidget(m_loopCount, 0, 1);
    lengthLayout->addWidget(new QLabel(tr("Fade length") + u":"_s, this), 1, 0);
    lengthLayout->addWidget(m_fadeLength, 1, 1);
    lengthLayout->setColumnStretch(2, 1);

    auto* metadataGroup  = new QGroupBox(tr("Metadata"), this);
    auto* metadataLayout = new QGridLayout(metadataGroup);
    metadataLayout->addWidget(m_generateTitles, 0, 0);

    auto* extensionsGroup  = new QGroupBox(tr("File types"), this);
    auto* extensionsLayout = new QGridLayout(extensionsGroup);
    m_commonExtensions->setToolTip(
        tr("Let VGMStream handle common formats such as OGG, FLAC and WAV."));
    extensionsLayout->addWidget(m_commonExtensions, 0, 0, 1, 2);

    auto* layout = new QGridLayout(this);
    layout->setSizeConstraint(QLayout::SetFixedSize);
    layout->addWidget(lengthGroup, 0, 0);
    layout->addWidget(metadataGroup, 1, 0);
    layout->addWidget(extensionsGroup, 2, 0);
    layout->addWidget(buttons, 3, 0, Qt::AlignBottom);

    m_loopCount->setValue(m_settings.value(LoopCount, DefaultLoopCount).toInt());
    m_fadeLength->setValue(m_settings.value(FadeLength, DefaultFadeLength).toInt());
    m_generateTitles->setChecked(m_settings.value(GenerateTitles, DefaultGenerateTitles).toBool());
    m_commonExtensions->setChecked(m_settings.value(CommonExtensions, DefaultCommonExtensions).toBool());
}

void VGMStreamSettings::accept()
{
    m_settings.setValue(LoopCount, m_loopCount->value());
    m_settings.setValue(FadeLength, m_fadeLength->value());
    m_settings.setValue(GenerateTitles, m_generateTitles->isChecked());
    m_settings.setValue(CommonExtensions, m_commonExtensions->isChecked());
    m_settings.sync();
    done(Accepted);
}
} // namespace Fooyin::VGMStream
