TEMPLATE = subdirs

SUBDIRS += AudioWaveformToneGenerator cpputils cpp-template-utils

AudioWaveformToneGenerator.file = app/AudioWaveformToneGenerator.pro
AudioWaveformToneGenerator.depends = cpputils cpp-template-utils
