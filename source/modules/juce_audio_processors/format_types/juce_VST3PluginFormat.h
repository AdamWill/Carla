/*
  ==============================================================================

   This file is part of the JUCE 7 technical preview.
   Copyright (c) 2022 - Raw Material Software Limited

   You may use this code under the terms of the GPL v3
   (see www.gnu.org/licenses).

   For the technical preview this file cannot be licensed commercially.

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

namespace juce
{

#if (JUCE_PLUGINHOST_VST3 && (JUCE_MAC || JUCE_WINDOWS || JUCE_LINUX || JUCE_BSD)) || DOXYGEN

/**
    Implements a plugin format for VST3s.

    @tags{Audio}
*/
class JUCE_API VST3PluginFormat   : public AudioPluginFormat
{
public:
    /** Constructor */
    VST3PluginFormat();

    /** Destructor */
    ~VST3PluginFormat() override;

    //==============================================================================
   #ifndef DOXYGEN
    /** Attempts to reload a VST3 plugin's state from some preset file data.

        @see VSTPluginFormat::loadFromFXBFile
    */
    [[deprecated ("Instead of using this function, use AudioPluginInstance::getExtensions() "
                 "to visit the ExtensionsVisitor::VST3 struct for the instance, if it exists. "
                 "Then, call ExtensionsVisitor::VST3::setPreset() to set the state using the "
                 "contents of a vstpreset file.")]]
    static bool setStateFromVSTPresetFile (AudioPluginInstance*, const MemoryBlock&);
   #endif

    //==============================================================================
    static String getFormatName()                   { return "VST3"; }
    String getName() const override                 { return getFormatName(); }
    bool canScanForPlugins() const override         { return true; }
    bool isTrivialToScan() const override           { return false; }

    void findAllTypesForFile (OwnedArray<PluginDescription>&, const String& fileOrIdentifier) override;
    bool fileMightContainThisPluginType (const String& fileOrIdentifier) override;
    String getNameOfPluginFromIdentifier (const String& fileOrIdentifier) override;
    bool pluginNeedsRescanning (const PluginDescription&) override;
    StringArray searchPathsForPlugins (const FileSearchPath&, bool recursive, bool) override;
    bool doesPluginStillExist (const PluginDescription&) override;
    FileSearchPath getDefaultLocationsToSearch() override;

private:
    //==============================================================================
    void createPluginInstance (const PluginDescription&, double initialSampleRate,
                               int initialBufferSize, PluginCreationCallback) override;
    bool requiresUnblockedMessageThreadDuringCreation (const PluginDescription&) const override;
    void recursiveFileSearch (StringArray&, const File&, bool recursive);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VST3PluginFormat)
};

#endif // JUCE_PLUGINHOST_VST3

} // namespace juce
