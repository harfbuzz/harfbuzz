from conans import ConanFile

class HarfbuzzConan(ConanFile):
    name = "harfbuzz"
    version = "3.3.2"
    url = "https://github.com/Esri/harfbuzz/tree/runtimecore"
    license = "https://github.com/Esri/harfbuzz/blob/master/COPYING"
    description = "HarfBuzz is a text shaping engine."

    # RTC specific triple
    settings = "platform_architecture_target"

    def package(self):
        base = self.source_folder + "/"
        relative = "3rdparty/harfbuzz/"

        # headers
        self.copy("*.h*", src=base + "src", dst=relative + "src")

        # libraries
        output = "output/" + str(self.settings.platform_architecture_target) + "/staticlib"
        self.copy("*" + self.name + "*", src=base + "../../" + output, dst=output)
