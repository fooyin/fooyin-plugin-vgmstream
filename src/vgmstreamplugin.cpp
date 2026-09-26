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

#include "vgmstreamplugin.h"

#include "vgmstreaminput.h"
#include "vgmstreamsettings.h"

#include <core/engine/audioloader.h>

using namespace Qt::StringLiterals;

namespace Fooyin::VGMStream {
namespace {
class VGMStreamSettingsProvider : public PluginSettingsProvider
{
public:
    explicit VGMStreamSettingsProvider(std::shared_ptr<AudioLoader> audioLoader)
        : m_audioLoader{std::move(audioLoader)}
    { }

protected:
    QDialog* createSettings(QWidget* parent) override
    {
        auto* dialog = new VGMStreamSettings(parent);
        QObject::connect(dialog, &QDialog::accepted, dialog, [audioLoader = m_audioLoader] {
                audioLoader->reloadDecoderExtensions(u"VGMStream"_s);
                audioLoader->reloadReaderExtensions(u"VGMStream"_s);
        });
        return dialog;
    }

private:
    std::shared_ptr<AudioLoader> m_audioLoader;
};
} // namespace

void VGMStreamPlugin::initialise(const CorePluginContext& context)
{
    m_audioLoader = context.audioLoader;
}

QString VGMStreamPlugin::inputName() const
{
    return u"VGMStream"_s;
}

InputCreator VGMStreamPlugin::inputCreator() const
{
    InputCreator creator;
    creator.priority = 200;
    creator.decoder  = []() {
        return std::make_unique<VGMStreamDecoder>();
    };
    creator.reader = []() {
        return std::make_unique<VGMStreamReader>();
    };
    return creator;
}

std::unique_ptr<PluginSettingsProvider> VGMStreamPlugin::settingsProvider() const
{
    return std::make_unique<VGMStreamSettingsProvider>(m_audioLoader);
}
} // namespace Fooyin::VGMStream

#include "moc_vgmstreamplugin.cpp"
