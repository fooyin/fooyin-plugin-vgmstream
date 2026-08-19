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

using namespace Qt::StringLiterals;

namespace Fooyin::VGMStream {
namespace {
class VGMStreamSettingsProvider : public PluginSettingsProvider
{
public:
    void showSettings(QWidget* parent) override
    {
        auto* dialog = new VGMStreamSettings(parent);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->show();
    }
};
} // namespace

QString VGMStreamPlugin::inputName() const
{
    return u"VGMStream"_s;
}

InputCreator VGMStreamPlugin::inputCreator() const
{
    InputCreator creator;
    creator.decoder = []() {
        return std::make_unique<VGMStreamDecoder>();
    };
    creator.reader = []() {
        return std::make_unique<VGMStreamReader>();
    };
    return creator;
}

std::unique_ptr<PluginSettingsProvider> VGMStreamPlugin::settingsProvider() const
{
    return std::make_unique<VGMStreamSettingsProvider>();
}
} // namespace Fooyin::VGMStream

#include "moc_vgmstreamplugin.cpp"
