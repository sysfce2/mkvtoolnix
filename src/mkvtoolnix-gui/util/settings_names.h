namespace {

// Groups:
char const * const s_grpDefaultJobSettings                  = "defaultJobSettings";
char const * const s_grpDefaults                            = "defaults";
char const * const s_grpDerivingTrackLanguagesFromFileNames = "derivingTrackLanguagesFromFileNames";
char const * const s_grpHeaderViewManager                   = "headerViewManager";
char const * const s_grpInfo                                = "info";
char const * const s_grpRunProgramConfigurations            = "runProgramConfigurations";
char const * const s_grpSettings                            = "settings";
char const * const s_grpSplitterSizes                       = "splitterSizes";
char const * const s_grpUpdates                             = "updates";
char const * const s_grpWindowGeometry                      = "windowGeometry";

// Values:
char const * const s_valActive                              = "active";
char const * const s_valAudioFile                           = "audioFile";
char const * const s_valAudioPolicy                         = "audioPolicy";
char const * const s_valAutoClearFileTitle                  = "autoClearFileTitle";
char const * const s_valAutoClearOutputFileName             = "autoClearOutputFileName";
char const * const s_valAutoDestinationOnlyForVideoFiles    = "autoDestinationOnlyForVideoFiles";
char const * const s_valAutoSetFileTitle                    = "autoSetFileTitle";
char const * const s_valCeTextFileCharacterSet              = "ceTextFileCharacterSet";
char const * const s_valChapterNameTemplate                 = "chapterNameTemplate";
char const * const s_valCheckForUpdates                     = "checkForUpdates";
char const * const s_valChecksums                           = "checksums";
char const * const s_valClearMergeSettings                  = "clearMergeSettings";
char const * const s_valColumnOrder                         = "columnOrder";
char const * const s_valColumnSizes                         = "columnSizes";
char const * const s_valCommandLine                         = "commandLine";
char const * const s_valCustomRegex                         = "customRegex";
char const * const s_valDefaultAdditionalMergeOptions       = "defaultAdditionalMergeOptions";
char const * const s_valDefaultAudioTrackLanguage           = "defaultAudioTrackLanguage";
char const * const s_valDefaultChapterCountry               = "defaultChapterCountry";
char const * const s_valDefaultChapterLanguage              = "defaultChapterLanguage";
char const * const s_valDefaultSubtitleCharset              = "defaultSubtitleCharset";
char const * const s_valDefaultSubtitleTrackLanguage        = "defaultSubtitleTrackLanguage";
char const * const s_valDefaultTrackLanguage                = "defaultTrackLanguage";
char const * const s_valDefaultVideoTrackLanguage           = "defaultVideoTrackLanguage";
char const * const s_valDisableCompressionForAllTrackTypes  = "disableCompressionForAllTrackTypes";
char const * const s_valDisableDefaultTrackForSubtitles     = "disableDefaultTrackForSubtitles";
char const * const s_valDropLastChapterFromBlurayPlaylist   = "dropLastChapterFromBlurayPlaylist";
char const * const s_valEnableMuxingAllAudioTracks          = "enableMuxingAllAudioTracks";
char const * const s_valEnableMuxingAllSubtitleTracks       = "enableMuxingAllSubtitleTracks";
char const * const s_valEnableMuxingAllVideoTracks          = "enableMuxingAllVideoTracks";
char const * const s_valEnableMuxingTracksByLanguage        = "enableMuxingTracksByLanguage";
char const * const s_valEnableMuxingTracksByTheseLanguages  = "enableMuxingTracksByTheseLanguages";
char const * const s_valEnableMuxingTracksByTheseTypes      = "enableMuxingTracksByTheseTypes";
char const * const s_valFixedOutputDir                      = "fixedOutputDir";
char const * const s_valForEvents                           = "forEvents";
char const * const s_valGuiVersion                          = "guiVersion";
char const * const s_valHeaderEditorDroppedFilesPolicy      = "headerEditorDroppedFilesPolicy";
char const * const s_valHexDumps                            = "hexDumps";
char const * const s_valHexPositions                        = "hexPositions";
char const * const s_valHiddenColumns                       = "hiddenColumns";
char const * const s_valJobRemovalPolicy                    = "jobRemovalPolicy";
char const * const s_valLastConfigDir                       = "lastConfigDir";
char const * const s_valLastOpenDir                         = "lastOpenDir";
char const * const s_valLastOutputDir                       = "lastOutputDir";
char const * const s_valLastUpdateCheck                     = "lastUpdateCheck";
char const * const s_valMediaInfoExe                        = "mediaInfoExe";
char const * const s_valMergeAddingAppendingFilesPolicy     = "mergeAddingAppendingFilesPolicy";
char const * const s_valMergeAlwaysAddDroppedFiles          = "mergeAlwaysAddDroppedFiles";
char const * const s_valMergeAlwaysShowOutputFileControls   = "mergeAlwaysShowOutputFileControls";
char const * const s_valMergeEnableDialogNormGainRemoval    = "mergeEnableDialogNormGainRemoval";
char const * const s_valMergeLastAddingAppendingDecision    = "mergeLastAddingAppendingDecision";
char const * const s_valMergeLastFixedOutputDirs            = "mergeLastFixedOutputDirs";
char const * const s_valMergeLastOutputDirs                 = "mergeLastOutputDirs";
char const * const s_valMergeLastRelativeOutputDirs         = "mergeLastRelativeOutputDirs";
char const * const s_valMergePredefinedSplitDurations       = "mergePredefinedSplitDurations";
char const * const s_valMergePredefinedSplitSizes           = "mergePredefinedSplitSizes";
char const * const s_valMergePredefinedTrackNames           = "mergePredefinedTrackNames";
char const * const s_valMergeTrackPropertiesLayout          = "mergeTrackPropertiesLayout";
char const * const s_valMergeUseVerticalInputLayout         = "mergeUseVerticalInputLayout";
char const * const s_valMergeWarnMissingAudioTrack          = "mergeWarnMissingAudioTrack";
char const * const s_valMinimumPlaylistDuration             = "minimumPlaylistDuration";
char const * const s_valMode                                = "mode";
char const * const s_valName                                = "name";
char const * const s_valOftenUsedCharacterSets              = "oftenUsedCharacterSets";
char const * const s_valOftenUsedCharacterSetsOnly          = "oftenUsedCharacterSetsOnly";
char const * const s_valOftenUsedCountries                  = "oftenUsedCountries";
char const * const s_valOftenUsedCountriesOnly              = "oftenUsedCountriesOnly";
char const * const s_valOftenUsedLanguages                  = "oftenUsedLanguages";
char const * const s_valOftenUsedLanguagesOnly              = "oftenUsedLanguagesOnly";
char const * const s_valOutputFileNamePolicy                = "outputFileNamePolicy";
char const * const s_valPriority                            = "priority";
char const * const s_valProbeRangePercentage                = "probeRangePercentage";
char const * const s_valRecognizedTrackLanguagesInFileNames = "recognizedTrackLanguagesInFileNames";
char const * const s_valRelativeOutputDir                   = "relativeOutputDir";
char const * const s_valRemoveOldJobs                       = "removeOldJobs";
char const * const s_valRemoveOldJobsDays                   = "removeOldJobsDays";
char const * const s_valRemoveOutputFileOnJobFailure        = "removeOutputFileOnJobFailure";
char const * const s_valResetJobWarningErrorCountersOnExit  = "resetJobWarningErrorCountersOnExit";
char const * const s_valScanForPlaylistsPolicy              = "scanForPlaylistsPolicy";
char const * const s_valSetAudioDelayFromFileName           = "setAudioDelayFromFileName";
char const * const s_valShowMoveUpDownButtons               = "showMoveUpDownButtons";
char const * const s_valShowOutputOfAllJobs                 = "showOutputOfAllJobs";
char const * const s_valShowToolSelector                    = "showToolSelector";
char const * const s_valSubtitlePolicy                      = "subtitlePolicy";
char const * const s_valSwitchToJobOutputAfterStarting      = "switchToJobOutputAfterStarting";
char const * const s_valTabPosition                         = "tabPosition";
char const * const s_valTrackInfo                           = "trackInfo";
char const * const s_valType                                = "type";
char const * const s_valUiDisableHighDPIScaling             = "uiDisableHighDPIScaling";
char const * const s_valUiDisableDarkStyleSheet             = "uiDisableDarkStyleSheet";
char const * const s_valUiFontFamily                        = "uiFontFamily";
char const * const s_valUiFontPointSize                     = "uiFontPointSize";
char const * const s_valUiLocale                            = "uiLocale";
char const * const s_valUniqueOutputFileNames               = "uniqueOutputFileNames";
char const * const s_valUseDefaultJobDescription            = "useDefaultJobDescription";
char const * const s_valVerbosity                           = "verbosity";
char const * const s_valVideoPolicy                         = "videoPolicy";
char const * const s_valVolume                              = "volume";
char const * const s_valWarnBeforeAbortingJobs              = "warnBeforeAbortingJobs";
char const * const s_valWarnBeforeClosingModifiedTabs       = "warnBeforeClosingModifiedTabs";
char const * const s_valWarnBeforeOverwriting               = "warnBeforeOverwriting";
char const * const s_valWhenToSetDefaultLanguage            = "whenToSetDefaultLanguage";

} // anonymous namespace
