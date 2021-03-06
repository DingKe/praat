/* praat_Sound_init.cpp
 *
 * Copyright (C) 1992-2012,2014,2015,2016 Paul Boersma
 *
 * This code is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This code is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this work. If not, see <http://www.gnu.org/licenses/>.
 */

#include "praat.h"

#include "Ltas.h"
#include "LongSound.h"
#include "Manipulation.h"
#include "ParamCurve.h"
#include "Sound_and_Spectrogram.h"
#include "Sound_and_Spectrum.h"
#include "Sound_extensions.h"
#include "Sound_to_Cochleagram.h"
#include "Sound_to_Formant.h"
#include "Sound_to_Harmonicity.h"
#include "Sound_to_Intensity.h"
#include "Sound_to_Pitch.h"
#include "Sound_to_PointProcess.h"
#include "SoundEditor.h"
#include "SoundRecorder.h"
#include "SpectrumEditor.h"
#include "TextGrid_Sound.h"
#include "mp3.h"

#undef iam
#define iam iam_LOOP

void praat_TimeFunction_query_init (ClassInfo klas);
void praat_TimeFunction_modify_init (ClassInfo klas);

/***** LONGSOUND *****/

DIRECT3 (INFO_LongSound_concatenate) {
	Melder_information (U"To concatenate LongSound objects, select them in the list\nand choose \"Save as WAV file...\" or a similar command.\n"
		"The result will be a sound file that contains\nthe concatenation of the selected sounds.");
END2 }

FORM3 (NEW_LongSound_extractPart, U"LongSound: Extract part", nullptr) {
	REAL (U"left Time range (s)", U"0.0")
	REAL (U"right Time range (s)", U"1.0")
	BOOLEAN (U"Preserve times", true)
	OK2
DO
	LOOP {
		iam (LongSound);
		autoSound thee = LongSound_extractPart (me, GET_REAL (U"left Time range"), GET_REAL (U"right Time range"), GET_INTEGER (U"Preserve times"));
		praat_new (thee.move(), my name);
	}
END2 }

FORM3 (REAL_LongSound_getIndexFromTime, U"LongSound: Get sample index from time", U"Sound: Get index from time...") {
	REAL (U"Time (s)", U"0.5")
	OK2
DO
	LOOP {
		iam (LongSound);
		double index = Sampled_xToIndex (me, GET_REAL (U"Time"));
		Melder_informationReal (index, nullptr);
	}
END2 }

DIRECT3 (REAL_LongSound_getSamplePeriod) {
	LOOP {
		iam (LongSound);
		Melder_informationReal (my dx, U"seconds");
	}
END2 }

DIRECT3 (REAL_LongSound_getSampleRate) {
	LOOP {
		iam (LongSound);
		Melder_informationReal (1.0 / my dx, U"Hz");
	}
END2 }

FORM3 (REAL_LongSound_getTimeFromIndex, U"LongSound: Get time from sample index", U"Sound: Get time from index...") {
	INTEGER (U"Sample index", U"100")
	OK2
DO
	LOOP {
		iam (LongSound);
		Melder_informationReal (Sampled_indexToX (me, GET_INTEGER (U"Sample index")), U"seconds");
	}
END2 }

DIRECT3 (INTEGER_LongSound_getNumberOfSamples) {
	LOOP {
		iam (LongSound);
		Melder_information (my nx, U" samples");
	}
END2 }

DIRECT3 (HELP_LongSound_help) { Melder_help (U"LongSound"); END2 }

FORM_READ3 (READ1_LongSound_open, U"Open long sound file", nullptr, true) {
	autoLongSound me = LongSound_open (file);
	praat_new (me.move(), MelderFile_name (file));
END2 }

FORM3 (PLAY_LongSound_playPart, U"LongSound: Play part", nullptr) {
	REAL (U"left Time range (s)", U"0.0")
	REAL (U"right Time range (s)", U"10.0")
	OK2
DO
	int n = 0;
	LOOP n ++;
	if (n == 1 || MelderAudio_getOutputMaximumAsynchronicity () < kMelder_asynchronicityLevel_ASYNCHRONOUS) {
		LOOP {
			iam (LongSound);
			LongSound_playPart (me, GET_REAL (U"left Time range"), GET_REAL (U"right Time range"), nullptr, nullptr);
		}
	} else {
		MelderAudio_setOutputMaximumAsynchronicity (kMelder_asynchronicityLevel_INTERRUPTABLE);
		LOOP {
			iam (LongSound);
			LongSound_playPart (me, GET_REAL (U"left Time range"), GET_REAL (U"right Time range"), nullptr, nullptr);
		}
		MelderAudio_setOutputMaximumAsynchronicity (kMelder_asynchronicityLevel_ASYNCHRONOUS);
	}
END2 }

FORM3 (SAVE_LongSound_savePartAsAudioFile, U"LongSound: Save part as audio file", nullptr) {
	LABEL (U"", U"Audio file:")
	TEXTFIELD (U"Audio file", U"")
	RADIO (U"Type", 3)
	{ int i; for (i = 1; i <= Melder_NUMBER_OF_AUDIO_FILE_TYPES; i ++) {
		RADIOBUTTON (Melder_audioFileTypeString (i))
	}}
	REAL (U"left Time range (s)", U"0.0")
	REAL (U"right Time range (s)", U"10.0")
	OK2
DO
	LOOP {
		iam (LongSound);
		structMelderFile file = { 0 };
		Melder_relativePathToFile (GET_STRING (U"Audio file"), & file);
		LongSound_savePartAsAudioFile (me, GET_INTEGER (U"Type"),
			GET_REAL (U"left Time range"), GET_REAL (U"right Time range"), & file, 16);
	}
END2 }
	
FORM3 (NEW_LongSound_to_TextGrid, U"LongSound: To TextGrid...", U"LongSound: To TextGrid...") {
	SENTENCE (U"Tier names", U"Mary John bell")
	SENTENCE (U"Point tiers", U"bell")
	OK2
DO
	LOOP {
		iam (LongSound);
		autoTextGrid thee = TextGrid_create (my xmin, my xmax, GET_STRING (U"Tier names"), GET_STRING (U"Point tiers"));
		praat_new (thee.move(), my name);
	}
END2 }

DIRECT3 (WINDOW_LongSound_view) {
	if (theCurrentPraatApplication -> batch) Melder_throw (U"Cannot view or edit a LongSound from batch.");
	LOOP {
		iam (LongSound);
		autoSoundEditor editor = SoundEditor_create (ID_AND_FULL_NAME, me);
		praat_installEditor (editor.get(), IOBJECT);
		editor.releaseToUser();
	}
END2 }

FORM_WRITE3 (SAVE_LongSound_saveAsAifcFile, U"Save as AIFC file", nullptr, U"aifc") {
	autoSoundAndLongSoundList list = SoundAndLongSoundList_create ();
	LOOP {
		iam (Sampled);
		list -> addItem_ref (me);
	}
	LongSound_concatenate (list.get(), file, Melder_AIFC, 16);
END2 }

FORM_WRITE3 (SAVE_LongSound_saveAsAiffFile, U"Save as AIFF file", nullptr, U"aiff") {
	autoSoundAndLongSoundList list = SoundAndLongSoundList_create ();
	LOOP {
		iam (Sampled);
		list -> addItem_ref (me);
	}
	LongSound_concatenate (list.get(), file, Melder_AIFF, 16);
END2 }

FORM_WRITE3 (SAVE_LongSound_saveAsNextSunFile, U"Save as NeXT/Sun file", nullptr, U"au") {
	autoSoundAndLongSoundList list = SoundAndLongSoundList_create ();
	LOOP {
		iam (Sampled);
		list -> addItem_ref (me);
	}
	LongSound_concatenate (list.get(), file, Melder_NEXT_SUN, 16);
END2 }

FORM_WRITE3 (SAVE_LongSound_saveAsNistFile, U"Save as NIST file", nullptr, U"nist") {
	autoSoundAndLongSoundList list = SoundAndLongSoundList_create ();
	LOOP {
		iam (Sampled);
		list -> addItem_ref (me);
	}
	LongSound_concatenate (list.get(), file, Melder_NIST, 16);
END2 }

FORM_WRITE3 (SAVE_LongSound_saveAsFlacFile, U"Save as FLAC file", nullptr, U"flac") {
	autoSoundAndLongSoundList list = SoundAndLongSoundList_create ();
	LOOP {
		iam (Sampled);
		list -> addItem_ref (me);
	}
	LongSound_concatenate (list.get(), file, Melder_FLAC, 16);
END2 }

FORM_WRITE3 (SAVE_LongSound_saveAsWavFile, U"Save as WAV file", nullptr, U"wav") {
	autoSoundAndLongSoundList list = SoundAndLongSoundList_create ();
	LOOP {
		iam (Sampled);
		list -> addItem_ref (me);
	}
	LongSound_concatenate (list.get(), file, Melder_WAV, 16);
END2 }

FORM_WRITE3 (SAVE_LongSound_saveLeftChannelAsAifcFile, U"Save left channel as AIFC file", nullptr, U"aifc") {
	LOOP {
		iam (LongSound);
		LongSound_saveChannelAsAudioFile (me, Melder_AIFC, 0, file);
	}
END2 }

FORM_WRITE3 (SAVE_LongSound_saveLeftChannelAsAiffFile, U"Save left channel as AIFF file", nullptr, U"aiff") {
	LOOP {
		iam (LongSound);
		LongSound_saveChannelAsAudioFile (me, Melder_AIFF, 0, file);
	}
END2 }

FORM_WRITE3 (SAVE_LongSound_saveLeftChannelAsNextSunFile, U"Save left channel as NeXT/Sun file", nullptr, U"au") {
	LOOP {
		iam (LongSound);
		LongSound_saveChannelAsAudioFile (me, Melder_NEXT_SUN, 0, file);
	}
END2 }

FORM_WRITE3 (SAVE_LongSound_saveLeftChannelAsNistFile, U"Save left channel as NIST file", nullptr, U"nist") {
	LOOP {
		iam (LongSound);
		LongSound_saveChannelAsAudioFile (me, Melder_NIST, 0, file);
	}
END2 }

FORM_WRITE3 (SAVE_LongSound_saveLeftChannelAsFlacFile, U"Save left channel as FLAC file", nullptr, U"flac") {
	LOOP {
		iam (LongSound);
		LongSound_saveChannelAsAudioFile (me, Melder_FLAC, 0, file);
	}
END2 }

FORM_WRITE3 (SAVE_LongSound_saveLeftChannelAsWavFile, U"Save left channel as WAV file", nullptr, U"wav") {
	LOOP {
		iam (LongSound);
		LongSound_saveChannelAsAudioFile (me, Melder_WAV, 0, file);
	}
END2 }

FORM_WRITE3 (SAVE_LongSound_saveRightChannelAsAifcFile, U"Save right channel as AIFC file", nullptr, U"aifc") {
	LOOP {
		iam (LongSound);
		LongSound_saveChannelAsAudioFile (me, Melder_AIFC, 1, file);
	}
END2 }

FORM_WRITE3 (SAVE_LongSound_saveRightChannelAsAiffFile, U"Save right channel as AIFF file", nullptr, U"aiff") {
	LOOP {
		iam (LongSound);
		LongSound_saveChannelAsAudioFile (me, Melder_AIFF, 1, file);
	}
END2 }

FORM_WRITE3 (SAVE_LongSound_saveRightChannelAsNextSunFile, U"Save right channel as NeXT/Sun file", nullptr, U"au") {
	LOOP {
		iam (LongSound);
		LongSound_saveChannelAsAudioFile (me, Melder_NEXT_SUN, 1, file);
	}
END2 }

FORM_WRITE3 (SAVE_LongSound_saveRightChannelAsNistFile, U"Save right channel as NIST file", nullptr, U"nist") {
	LOOP {
		iam (LongSound);
		LongSound_saveChannelAsAudioFile (me, Melder_NIST, 1, file);
	}
END2 }

FORM_WRITE3 (SAVE_LongSound_saveRightChannelAsFlacFile, U"Save right channel as FLAC file", 0, U"flac") {
	LOOP {
		iam (LongSound);
		LongSound_saveChannelAsAudioFile (me, Melder_FLAC, 1, file);
	}
END2 }

FORM_WRITE3 (SAVE_LongSound_saveRightChannelAsWavFile, U"Save right channel as WAV file", 0, U"wav") {
	LOOP {
		iam (LongSound);
		LongSound_saveChannelAsAudioFile (me, Melder_WAV, 1, file);
	}
END2 }

FORM3 (PREFS_LongSoundPrefs, U"LongSound preferences", U"LongSound") {
	LABEL (U"", U"This setting determines the maximum number of seconds")
	LABEL (U"", U"for viewing the waveform and playing a sound in the LongSound window.")
	LABEL (U"", U"The LongSound window can become very slow if you set it too high.")
	NATURAL (U"Maximum viewable part (seconds)", U"60")
	LABEL (U"", U"Note: this setting works for the next long sound file that you open,")
	LABEL (U"", U"not for currently existing LongSound objects.")
	OK2
SET_INTEGER (U"Maximum viewable part", LongSound_getBufferSizePref_seconds ())
DO
	LongSound_setBufferSizePref_seconds (GET_INTEGER (U"Maximum viewable part"));
END2 }

/********** LONGSOUND & SOUND **********/

FORM_WRITE3 (SAVE_LongSound_Sound_saveAsAifcFile, U"Save as AIFC file", nullptr, U"aifc") {
	autoSoundAndLongSoundList list = SoundAndLongSoundList_create ();
	LOOP {
		iam (Sampled);
		list -> addItem_ref (me);
	}
	LongSound_concatenate (list.get(), file, Melder_AIFC, 16);
END2 }

FORM_WRITE3 (SAVE_LongSound_Sound_saveAsAiffFile, U"Save as AIFF file", nullptr, U"aiff") {
	autoSoundAndLongSoundList list = SoundAndLongSoundList_create ();
	LOOP {
		iam (Sampled);
		list -> addItem_ref (me);
	}
	LongSound_concatenate (list.get(), file, Melder_AIFF, 16);
END2 }

FORM_WRITE3 (SAVE_LongSound_Sound_saveAsNextSunFile, U"Save as NeXT/Sun file", nullptr, U"au") {
	autoSoundAndLongSoundList list = SoundAndLongSoundList_create ();
	LOOP {
		iam (Sampled);
		list -> addItem_ref (me);
	}
	LongSound_concatenate (list.get(), file, Melder_NEXT_SUN, 16);
END2 }

FORM_WRITE3 (SAVE_LongSound_Sound_saveAsNistFile, U"Save as NIST file", nullptr, U"nist") {
	autoSoundAndLongSoundList list = SoundAndLongSoundList_create ();
	LOOP {
		iam (Sampled);
		list -> addItem_ref (me);
	}
	LongSound_concatenate (list.get(), file, Melder_NIST, 16);
END2 }

FORM_WRITE3 (SAVE_LongSound_Sound_saveAsFlacFile, U"Save as FLAC file", nullptr, U"flac") {
	autoSoundAndLongSoundList list = SoundAndLongSoundList_create ();
	LOOP {
		iam (Sampled);
		list -> addItem_ref (me);
	}
	LongSound_concatenate (list.get(), file, Melder_FLAC, 16);
END2 }

FORM_WRITE3 (SAVE_LongSound_Sound_saveAsWavFile, U"Save as WAV file", nullptr, U"wav") {
	autoSoundAndLongSoundList list = SoundAndLongSoundList_create ();
	LOOP {
		iam (Sampled);
		list -> addItem_ref (me);
	}
	LongSound_concatenate (list.get(), file, Melder_WAV, 16);
END2 }

/********** SOUND **********/

FORM3 (MODIFY_Sound_add, U"Sound: Add", nullptr) {
	LABEL (U"", U"The following number will be added to the amplitudes of ")
	LABEL (U"", U"all samples of the sound.")
	REAL (U"Number", U"0.1")
	OK2
DO
	LOOP {
		iam (Sound);
		Vector_addScalar (me, GET_REAL (U"Number"));
		praat_dataChanged (me);
	}
END2 }

FORM3 (NEW_Sound_autoCorrelate, U"Sound: autocorrelate", U"Sound: Autocorrelate...") {
	RADIO_ENUM (U"Amplitude scaling", kSounds_convolve_scaling, DEFAULT)
	RADIO_ENUM (U"Signal outside time domain is...", kSounds_convolve_signalOutsideTimeDomain, DEFAULT)
 	OK2
DO
	LOOP {
		iam (Sound);
		autoSound thee = Sound_autoCorrelate (me,
			GET_ENUM (kSounds_convolve_scaling, U"Amplitude scaling"),
			GET_ENUM (kSounds_convolve_signalOutsideTimeDomain, U"Signal outside time domain is..."));
		praat_new (thee.move(), U"ac_", my name);
	}
END2 }

DIRECT3 (NEW1_Sounds_combineToStereo) {
	OrderedOf<structSound> list;
	LOOP {
		iam (Sound);
		list. addItem_ref (me);
	}
	autoSound result = Sounds_combineToStereo (& list);
	long numberOfChannels = result -> ny;   // dereference before transferring
	praat_new (result.move(), U"combined_", numberOfChannels);
END2 }

DIRECT3 (NEW1_Sounds_concatenate) {
	OrderedOf<structSound> list;
	LOOP {
		iam_LOOP (Sound);
		list.addItem_ref (me);
	}
	autoSound result = Sounds_concatenate (list, 0.0);
	praat_new (result.move(), U"chain");
END2 }

FORM3 (NEW1_Sounds_concatenateWithOverlap, U"Sounds: Concatenate with overlap", U"Sounds: Concatenate with overlap...") {
	POSITIVE (U"Overlap (s)", U"0.01")
	OK2
DO
	OrderedOf<structSound> list;
	LOOP {
		iam_LOOP (Sound);
		list.addItem_ref (me);
	}
	autoSound result = Sounds_concatenate (list, GET_REAL (U"Overlap"));
	praat_new (result.move(), U"chain");
END2 }

DIRECT3 (NEW2_Sounds_concatenateRecoverably) {
	long numberOfChannels = 0, nx = 0, iinterval = 0;
	double dx = 0.0, tmin = 0.0;
	LOOP {
		iam (Sound);
		if (numberOfChannels == 0) {
			numberOfChannels = my ny;
		} else if (my ny != numberOfChannels) {
			Melder_throw (U"To concatenate sounds, their numbers of channels (mono, stereo) must be equal.");
		}
		if (dx == 0.0) {
			dx = my dx;
		} else if (my dx != dx) {
			Melder_throw (U"To concatenate sounds, their sampling frequencies must be equal.\n"
				"You could resample one or more of the sounds before concatenating.");
		}
		nx += my nx;
	}
	autoSound thee = Sound_create (numberOfChannels, 0.0, nx * dx, nx, dx, 0.5 * dx);
	autoTextGrid him = TextGrid_create (0.0, nx * dx, U"labels", U"");
	nx = 0;
	LOOP {
		iam (Sound);
		double tmax = tmin + my nx * dx;
		for (long channel = 1; channel <= numberOfChannels; channel ++) {
			NUMvector_copyElements (my z [channel], thy z [channel] + nx, 1, my nx);
		}
		iinterval ++;
		if (iinterval > 1) {
			TextGrid_insertBoundary (him.get(), 1, tmin);
		}
		TextGrid_setIntervalText (him.get(), 1, iinterval, my name);
		nx += my nx;
		tmin = tmax;
	}
	praat_new (thee.move(), U"chain");
	praat_new (him.move(), U"chain");
END2 }

DIRECT3 (NEW_Sound_convertToMono) {
	LOOP {
		iam (Sound);
		autoSound thee = Sound_convertToMono (me);
		praat_new (thee.move(), my name, U"_mono");
	}
END2 }

DIRECT3 (NEW_Sound_convertToStereo) {
	LOOP {
		iam (Sound);
		autoSound thee = Sound_convertToStereo (me);
		praat_new (thee.move(), my name, U"_stereo");
	}
END2 }

DIRECT3 (NEW1_Sounds_convolve_old) {
	Sound s1 = nullptr, s2 = nullptr;
	LOOP {
		iam (Sound);
		( s1 ? s2 : s1 ) = me;
	}
	Melder_assert (s1 && s2);
	autoSound thee = Sounds_convolve (s1, s2, kSounds_convolve_scaling_SUM, kSounds_convolve_signalOutsideTimeDomain_ZERO);
	praat_new (thee.move(), s1 -> name, U"_", s2 -> name);
END2 }

FORM3 (NEW1_Sounds_convolve, U"Sounds: Convolve", U"Sounds: Convolve...") {
	RADIO_ENUM (U"Amplitude scaling", kSounds_convolve_scaling, DEFAULT)
	RADIO_ENUM (U"Signal outside time domain is...", kSounds_convolve_signalOutsideTimeDomain, DEFAULT)
	OK2
DO
	Sound s1 = nullptr, s2 = nullptr;
	LOOP {
		iam (Sound);
		( s1 ? s2 : s1 ) = me;
	}
	Melder_assert (s1 && s2);
	autoSound thee = Sounds_convolve (s1, s2,
		GET_ENUM (kSounds_convolve_scaling, U"Amplitude scaling"),
		GET_ENUM (kSounds_convolve_signalOutsideTimeDomain, U"Signal outside time domain is..."));
	praat_new (thee.move(), s1 -> name, U"_", s2 -> name);
END2 }

static void common_Sound_create (UiForm dia, Interpreter interpreter, bool allowMultipleChannels) {
	long numberOfChannels = allowMultipleChannels ? GET_INTEGER (U"Number of channels") : 1;
	double startTime = GET_REAL (U"Start time");
	double endTime = GET_REAL (U"End time");
	double samplingFrequency = GET_REAL (U"Sampling frequency");
	double numberOfSamples_real = round ((endTime - startTime) * samplingFrequency);
	if (endTime <= startTime) {
		if (endTime == startTime)
			Melder_appendError (U"A Sound cannot have a duration of zero.");
		else
			Melder_appendError (U"A Sound cannot have a duration less than zero.");
		if (startTime == 0.0)
			Melder_throw (U"Please set the end time to something greater than 0 seconds.");
		else
			Melder_throw (U"Please lower the start time or raise the end time.");
	}
	if (samplingFrequency <= 0.0) {
		Melder_appendError (U"A Sound cannot have a negative sampling frequency.");
		Melder_throw (U"Please set the sampling frequency to something greater than zero, e.g. 44100 Hz.");
	}
	if (numberOfChannels < 1)
		Melder_throw (U"A Sound cannot have zero channels.");
	if (numberOfSamples_real < 1.0) {
		Melder_appendError (U"A Sound cannot have zero samples.");
		if (startTime == 0.0)
			Melder_throw (U"Please raise the end time.");
		else
			Melder_throw (U"Please lower the start time or raise the end time.");
	}
	if (numberOfSamples_real > INT54_MAX) {
		Melder_appendError (U"A Sound cannot have ", numberOfSamples_real, U" samples; the maximum is ",
			Melder_bigInteger (INT54_MAX), U" samples (or less, depending on your computer's memory).");
		if (startTime == 0.0)
			Melder_throw (U"Please lower the end time or the sampling frequency.");
		else
			Melder_throw (U"Please raise the start time, lower the end time, or lower the sampling frequency.");
	}
	int64 numberOfSamples = (int64) numberOfSamples_real;
	autoSound sound;
	try {
		sound = Sound_create (numberOfChannels, startTime, endTime, numberOfSamples, 1.0 / samplingFrequency,
			startTime + 0.5 * (endTime - startTime - (numberOfSamples - 1) / samplingFrequency));
	} catch (MelderError) {
		if (str32str (Melder_getError (), U"memory")) {
			Melder_clearError ();
			Melder_appendError (U"There is not enough memory to create a Sound that contains ", Melder_bigInteger (numberOfSamples), U" samples.");
			if (startTime == 0.0)
				Melder_throw (U"You could lower the end time or the sampling frequency and try again.");
			else
				Melder_throw (U"You could raise the start time or lower the end time or the sampling frequency, and try again.");
		} else {
			throw;   // unexpected error; wait for generic message
		}
	}
	Matrix_formula (sound.get(), GET_STRING (U"formula"), interpreter, nullptr);
	praat_new (sound.move(), GET_STRING (U"Name"));
	//praat_updateSelection ();
}

FORM3 (NEW1_Sound_create, U"Create mono Sound", U"Create Sound from formula...") {
	WORD (U"Name", U"sineWithNoise")
	REAL (U"Start time (s)", U"0.0")
	REAL (U"End time (s)", U"1.0")
	REAL (U"Sampling frequency (Hz)", U"44100")
	LABEL (U"", U"Formula:")
	TEXTFIELD (U"formula", U"1/2 * sin(2*pi*377*x) + randomGauss(0,0.1)")
	OK2
DO
	common_Sound_create (dia, interpreter, false);
END2 }

FORM3 (NEW1_Sound_createFromFormula, U"Create Sound from formula", U"Create Sound from formula...") {
	WORD (U"Name", U"sineWithNoise")
	CHANNEL (U"Number of channels", U"1 (= mono)")
	REAL (U"Start time (s)", U"0.0")
	REAL (U"End time (s)", U"1.0")
	REAL (U"Sampling frequency (Hz)", U"44100")
	LABEL (U"", U"Formula:")
	TEXTFIELD (U"formula", U"1/2 * sin(2*pi*377*x) + randomGauss(0,0.1)")
	OK2
DO
	common_Sound_create (dia, interpreter, true);
END2 }

FORM3 (NEW1_Sound_createAsPureTone, U"Create Sound as pure tone", U"Create Sound as pure tone...") {
	WORD (U"Name", U"tone")
	CHANNEL (U"Number of channels", U"1 (= mono)")
	REAL (U"Start time (s)", U"0.0")
	REAL (U"End time (s)", U"0.4")
	POSITIVE (U"Sampling frequency (Hz)", U"44100.0")
	POSITIVE (U"Tone frequency (Hz)", U"440.0")
	POSITIVE (U"Amplitude (Pa)", U"0.2")
	POSITIVE (U"Fade-in duration (s)", U"0.01")
	POSITIVE (U"Fade-out duration (s)", U"0.01")
	OK2
DO
	autoSound me = Sound_createAsPureTone (GET_INTEGER (U"Number of channels"), GET_REAL (U"Start time"), GET_REAL (U"End time"),
		GET_REAL (U"Sampling frequency"), GET_REAL (U"Tone frequency"), GET_REAL (U"Amplitude"),
		GET_REAL (U"Fade-in duration"), GET_REAL (U"Fade-out duration"));
	praat_new (me.move(), GET_STRING (U"Name"));
END2 }

FORM3 (NEW1_Sound_createFromToneComplex, U"Create Sound from tone complex", U"Create Sound from tone complex...") {
	WORD (U"Name", U"toneComplex")
	REAL (U"Start time (s)", U"0.0")
	REAL (U"End time (s)", U"1.0")
	POSITIVE (U"Sampling frequency (Hz)", U"44100.0")
	RADIO (U"Phase", 2)
		RADIOBUTTON (U"sine")
		RADIOBUTTON (U"cosine")
	POSITIVE (U"Frequency step (Hz)", U"100.0")
	REAL (U"First frequency (Hz)", U"0.0 (= frequency step)")
	REAL (U"Ceiling (Hz)", U"0.0 (= Nyquist)")
	INTEGER (U"Number of components", U"0 (= maximum)")
	OK2
DO
	autoSound me = Sound_createFromToneComplex (GET_REAL (U"Start time"), GET_REAL (U"End time"),
		GET_REAL (U"Sampling frequency"), GET_INTEGER (U"Phase") - 1, GET_REAL (U"Frequency step"),
		GET_REAL (U"First frequency"), GET_REAL (U"Ceiling"), GET_INTEGER (U"Number of components"));
	praat_new (me.move(), GET_STRING (U"Name"));
END2 }

FORM3 (NEW1_old_Sounds_crossCorrelate, U"Cross-correlate (short)", nullptr) {
	REAL (U"From lag (s)", U"-0.1")
	REAL (U"To lag (s)", U"0.1")
	BOOLEAN (U"Normalize", true)
	OK2
DO
	Sound s1 = nullptr, s2 = nullptr;
	LOOP {
		iam (Sound);
		( s1 ? s2 : s1 ) = me;
	}
	autoSound thee = Sounds_crossCorrelate_short (s1, s2, GET_REAL (U"From lag"), GET_REAL (U"To lag"), GET_INTEGER (U"Normalize"));
	praat_new (thee.move(), U"cc_", s1 -> name, U"_", s2 -> name);
END2 }

FORM3 (NEW1_Sounds_crossCorrelate, U"Sounds: Cross-correlate", U"Sounds: Cross-correlate...") {
	RADIO_ENUM (U"Amplitude scaling", kSounds_convolve_scaling, DEFAULT)
	RADIO_ENUM (U"Signal outside time domain is...", kSounds_convolve_signalOutsideTimeDomain, DEFAULT)
	OK2
DO_ALTERNATIVE3 (NEW1_old_Sounds_crossCorrelate)
	Sound s1 = nullptr, s2 = nullptr;
	LOOP {
		iam (Sound);
		( s1 ? s2 : s1 ) = me;
	}
	Melder_assert (s1 && s2);
	autoSound thee = Sounds_crossCorrelate (s1, s2,
		GET_ENUM (kSounds_convolve_scaling, U"Amplitude scaling"),
		GET_ENUM (kSounds_convolve_signalOutsideTimeDomain, U"Signal outside time domain is..."));
	praat_new (thee.move(), s1 -> name, U"_", s2 -> name);
END2 }

FORM3 (MODIFY_Sound_deemphasizeInline, U"Sound: De-emphasize (in-line)", U"Sound: De-emphasize (in-line)...") {
	REAL (U"From frequency (Hz)", U"50.0")
	OK2
DO
	LOOP {
		iam (Sound);
		Sound_deEmphasis (me, GET_REAL (U"From frequency"));
		Vector_scale (me, 0.99);
		praat_dataChanged (me);
	}
END2 }

FORM3 (NEW_Sound_deepenBandModulation, U"Deepen band modulation", U"Sound: Deepen band modulation...") {
	POSITIVE (U"Enhancement (dB)", U"20")
	POSITIVE (U"From frequency (Hz)", U"300")
	POSITIVE (U"To frequency (Hz)", U"8000")
	POSITIVE (U"Slow modulation (Hz)", U"3")
	POSITIVE (U"Fast modulation (Hz)", U"30")
	POSITIVE (U"Band smoothing (Hz)", U"100")
	OK2
DO
	LOOP {
		iam (Sound);
		autoSound thee = Sound_deepenBandModulation (me, GET_REAL (U"Enhancement"),
			GET_REAL (U"From frequency"), GET_REAL (U"To frequency"),
			GET_REAL (U"Slow modulation"), GET_REAL (U"Fast modulation"), GET_REAL (U"Band smoothing"));
		praat_new (thee.move(), my name, U"_", (long) (GET_REAL (U"Enhancement")));   // truncate number toward zero for visual effect
	}
END2 }

FORM3 (GRAPHICS_old_Sound_draw, U"Sound: Draw", nullptr) {
	REAL (U"left Time range (s)", U"0.0")
	REAL (U"right Time range", U"0.0 (= all)")
	REAL (U"left Vertical range", U"0.0")
	REAL (U"right Vertical range", U"0.0 (= auto)")
	BOOLEAN (U"Garnish", true)
	OK2
DO
	autoPraatPicture picture;
	LOOP {
		iam (Sound);
		Sound_draw (me, GRAPHICS, GET_REAL (U"left Time range"), GET_REAL (U"right Time range"),
			GET_REAL (U"left Vertical range"), GET_REAL (U"right Vertical range"), GET_INTEGER (U"Garnish"), U"curve");
	}
END2 }

FORM3 (GRAPHICS_Sound_draw, U"Sound: Draw", nullptr) {
	REAL (U"left Time range (s)", U"0.0")
	REAL (U"right Time range", U"0.0 (= all)")
	REAL (U"left Vertical range", U"0.0")
	REAL (U"right Vertical range", U"0.0 (= auto)")
	BOOLEAN (U"Garnish", true)
	LABEL (U"", U"")
	OPTIONMENU (U"Drawing method", 1)
		OPTION (U"Curve")
		OPTION (U"Bars")
		OPTION (U"Poles")
		OPTION (U"Speckles")
	OK2
DO_ALTERNATIVE3 (GRAPHICS_old_Sound_draw)
	autoPraatPicture picture;
	LOOP {
		iam (Sound);
		Sound_draw (me, GRAPHICS, GET_REAL (U"left Time range"), GET_REAL (U"right Time range"),
			GET_REAL (U"left Vertical range"), GET_REAL (U"right Vertical range"), GET_INTEGER (U"Garnish"), GET_STRING (U"Drawing method"));
	}
END2 }

static void cb_SoundEditor_publication (Editor /* me */, autoDaata publication) {
	/*
	 * Keep the gate for error handling.
	 */
	try {
		bool isaSpectrum = Thing_isa (publication.get(), classSpectrum);
		praat_new (publication.move(), U"");
		praat_updateSelection ();
		if (isaSpectrum) {
			int IOBJECT;
			LOOP {
				iam (Spectrum);
				autoSpectrumEditor editor2 = SpectrumEditor_create (ID_AND_FULL_NAME, me);
				praat_installEditor (editor2.get(), IOBJECT);
				editor2.releaseToUser();
			}
		}
	} catch (MelderError) {
		Melder_flushError ();
	}
}
DIRECT3 (WINDOW_Sound_viewAndEdit) {
	if (theCurrentPraatApplication -> batch) Melder_throw (U"Cannot view or edit a Sound from batch.");
	LOOP {
		iam (Sound);
		autoSoundEditor editor = SoundEditor_create (ID_AND_FULL_NAME, me);
		Editor_setPublicationCallback (editor.get(), cb_SoundEditor_publication);
		praat_installEditor (editor.get(), IOBJECT);
		editor.releaseToUser();
	}
END2 }

DIRECT3 (NEWMANY_Sound_extractAllChannels) {
	LOOP {
		iam (Sound);
		for (long channel = 1; channel <= my ny; channel ++) {
			autoSound thee = Sound_extractChannel (me, channel);
			praat_new (thee.move(), my name, U"_ch", channel);
		}
	}
END2 }

FORM3 (NEW_Sound_extractChannel, U"Sound: Extract channel", nullptr) {
	CHANNEL (U"Channel (number, Left, or Right)", U"1")
	OK2
DO
	long channel = GET_INTEGER (U"Channel");
	LOOP {
		iam (Sound);
		autoSound thee = Sound_extractChannel (me, channel);
		praat_new (thee.move(), my name, U"_ch", channel);
	}
END2 }

DIRECT3 (NEW_Sound_extractLeftChannel) {
	LOOP {
		iam (Sound);
		autoSound thee = Sound_extractChannel (me, 1);
		praat_new (thee.move(), my name, U"_left");
	}
END2 }

FORM3 (NEW_Sound_extractPart, U"Sound: Extract part", nullptr) {
	REAL (U"left Time range (s)", U"0.0")
	REAL (U"right Time range (s)", U"0.1")
	OPTIONMENU_ENUM (U"Window shape", kSound_windowShape, DEFAULT)
	POSITIVE (U"Relative width", U"1.0")
	BOOLEAN (U"Preserve times", false)
	OK2
DO
	LOOP {
		iam (Sound);
		autoSound thee = Sound_extractPart (me,
			GET_REAL (U"left Time range"), GET_REAL (U"right Time range"),
			GET_ENUM (kSound_windowShape, U"Window shape"), GET_REAL (U"Relative width"),
			GET_INTEGER (U"Preserve times"));
		praat_new (thee.move(), my name, U"_part");
	}
END2 }

FORM3 (NEW_Sound_extractPartForOverlap, U"Sound: Extract part for overlap", nullptr) {
	REAL (U"left Time range (s)", U"0.0")
	REAL (U"right Time range (s)", U"0.1")
	POSITIVE (U"Overlap (s)", U"0.01")
	OK2
DO
	LOOP {
		iam (Sound);
		autoSound thee = Sound_extractPartForOverlap (me,
			GET_REAL (U"left Time range"), GET_REAL (U"right Time range"),
			GET_REAL (U"Overlap"));
		praat_new (thee.move(), my name, U"_part");
	}
END2 }

DIRECT3 (NEW_Sound_extractRightChannel) {
	LOOP {
		iam (Sound);
		autoSound thee = Sound_extractChannel (me, 2);
		praat_new (thee.move(), my name, U"_right");
	}
END2 }

FORM3 (NEW_Sound_filter_deemphasis, U"Sound: Filter (de-emphasis)", U"Sound: Filter (de-emphasis)...") {
	REAL (U"From frequency (Hz)", U"50.0")
	OK2
DO
	LOOP {
		iam (Sound);
		autoSound thee = Sound_filter_deemphasis (me, GET_REAL (U"From frequency"));
		praat_new (thee.move(), my name, U"_deemp");
	}
END2 }

FORM3 (NEW_Sound_filter_formula, U"Sound: Filter (formula)...", U"Formula...") {
	LABEL (U"", U"Frequency-domain filtering with a formula (uses Sound-to-Spectrum and Spectrum-to-Sound): x is frequency in hertz")
	TEXTFIELD (U"formula", U"if x<500 or x>1000 then 0 else self fi; rectangular band filter")
	OK2
DO
	LOOP {
		iam (Sound);
		autoSound thee = Sound_filter_formula (me, GET_STRING (U"formula"), interpreter);
		praat_new (thee.move(), my name, U"_filt");
	}
END2 }

FORM3 (NEW_Sound_filter_oneFormant, U"Sound: Filter (one formant)", U"Sound: Filter (one formant)...") {
	REAL (U"Frequency (Hz)", U"1000.0")
	POSITIVE (U"Bandwidth (Hz)", U"100.0")
	OK2
DO
	LOOP {
		iam (Sound);
		autoSound thee = Sound_filter_oneFormant (me, GET_REAL (U"Frequency"), GET_REAL (U"Bandwidth"));
		praat_new (thee.move(), my name, U"_filt");
	}
END2 }

FORM3 (MODIFY_Sound_filterWithOneFormantInline, U"Sound: Filter with one formant (in-line)", U"Sound: Filter with one formant (in-line)...") {
	REAL (U"Frequency (Hz)", U"1000.0")
	POSITIVE (U"Bandwidth (Hz)", U"100.0")
	OK2
DO
	LOOP {
		iam (Sound);
		Sound_filterWithOneFormantInline (me, GET_REAL (U"Frequency"), GET_REAL (U"Bandwidth"));
		praat_dataChanged (me);
	}
END2 }

FORM3 (NEW_Sound_filter_passHannBand, U"Sound: Filter (pass Hann band)", U"Sound: Filter (pass Hann band)...") {
	REAL (U"From frequency (Hz)", U"500.0")
	REAL (U"To frequency (Hz)", U"1000.0")
	POSITIVE (U"Smoothing (Hz)", U"100.0")
	OK2
DO
	LOOP {
		iam (Sound);
		autoSound thee = Sound_filter_passHannBand (me,
			GET_REAL (U"From frequency"), GET_REAL (U"To frequency"), GET_REAL (U"Smoothing"));
		praat_new (thee.move(), my name, U"_band");
	}
END2 }

FORM3 (NEW_Sound_filter_preemphasis, U"Sound: Filter (pre-emphasis)", U"Sound: Filter (pre-emphasis)...") {
	REAL (U"From frequency (Hz)", U"50.0")
	OK2
DO
	LOOP {
		iam (Sound);
		autoSound thee = Sound_filter_preemphasis (me, GET_REAL (U"From frequency"));
		praat_new (thee.move(), my name, U"_preemp");
	}
END2 }

FORM3 (NEW_Sound_filter_stopHannBand, U"Sound: Filter (stop Hann band)", U"Sound: Filter (stop Hann band)...") {
	REAL (U"From frequency (Hz)", U"500.0")
	REAL (U"To frequency (Hz)", U"1000.0")
	POSITIVE (U"Smoothing (Hz)", U"100.0")
	OK2
DO
	LOOP {
		iam (Sound);
		autoSound thee = Sound_filter_stopHannBand (me, GET_REAL (U"From frequency"), GET_REAL (U"To frequency"), GET_REAL (U"Smoothing"));
		praat_new (thee.move(), my name, U"_band");
	}
END2 }

FORM3 (MODIFY_Sound_formula, U"Sound: Formula", U"Sound: Formula...") {
	LABEL (U"label1", U"! `x' is the time in seconds, `col' is the sample number.")
	LABEL (U"label2", U"x = x1   ! time associated with first sample")
	LABEL (U"label3", U"for col from 1 to ncol")
	LABEL (U"label4", U"   self [col] = ...")
	TEXTFIELD (U"formula", U"self")
	LABEL (U"label5", U"   x = x + dx")
	LABEL (U"label6", U"endfor")
	OK2
DO
	LOOP {
		iam (Sound);
		try {
			Matrix_formula (me, GET_STRING (U"formula"), interpreter, nullptr);
			praat_dataChanged (me);
		} catch (MelderError) {
			praat_dataChanged (me);   // in case of error, the Sound may have partially changed
			throw;
		}
	}
END2 }

FORM3 (MODIFY_Sound_formula_part, U"Sound: Formula (part)", U"Sound: Formula...") {
	REAL (U"From time", U"0.0")
	REAL (U"To time", U"0.0 (= all)")
	NATURAL (U"From channel", U"1")
	NATURAL (U"To channel", U"2")
	TEXTFIELD (U"formula", U"2 * self")
	OK2
DO
	LOOP {
		iam (Sound);
		try {
			Matrix_formula_part (me,
				GET_REAL (U"From time"), GET_REAL (U"To time"),
				GET_INTEGER (U"From channel") - 0.5, GET_INTEGER (U"To channel") + 0.5,
				GET_STRING (U"formula"), interpreter, nullptr);
			praat_dataChanged (me);
		} catch (MelderError) {
			praat_dataChanged (me);   // in case of error, the Sound may have partially changed
			throw;
		}
	}
END2 }

FORM3 (REAL_Sound_getAbsoluteExtremum, U"Sound: Get absolute extremum", U"Sound: Get absolute extremum...") {
	REAL (U"left Time range (s)", U"0.0")
	REAL (U"right Time range (s)", U"0.0 (= all)")
	RADIO (U"Interpolation", 4)
		RADIOBUTTON (U"None")
		RADIOBUTTON (U"Parabolic")
		RADIOBUTTON (U"Cubic")
		RADIOBUTTON (U"Sinc70")
		RADIOBUTTON (U"Sinc700")
	OK2
DO
	LOOP {
		iam (Sound);
		double absoluteExtremum = Vector_getAbsoluteExtremum (me,
			GET_REAL (U"left Time range"), GET_REAL (U"right Time range"), GET_INTEGER (U"Interpolation") - 1);
		Melder_informationReal (absoluteExtremum, U"Pascal");
	}
END2 }

FORM3 (REAL_Sound_getEnergy, U"Sound: Get energy", U"Sound: Get energy...") {
	REAL (U"left Time range (s)", U"0.0")
	REAL (U"right Time range (s)", U"0.0 (= all)")
	OK2
DO
	LOOP {
		iam (Sound);
		double energy = Sound_getEnergy (me, GET_REAL (U"left Time range"), GET_REAL (U"right Time range"));
		Melder_informationReal (energy, U"Pa2 sec");
	}
END2 }

DIRECT3 (REAL_Sound_getEnergyInAir) {
	LOOP {
		iam (Sound);
		double energyInAir = Sound_getEnergyInAir (me);
		Melder_informationReal (energyInAir, U"Joule/m2");
	}
END2 }

FORM3 (REAL_Sound_getIndexFromTime, U"Get sample number from time", U"Get sample number from time...") {
	REAL (U"Time (s)", U"0.5")
	OK2
DO
	LOOP {
		iam (Sound);
		double realIndex = Sampled_xToIndex (me, GET_REAL (U"Time"));
		Melder_informationReal (realIndex, nullptr);
	}
END2 }

DIRECT3 (REAL_Sound_getIntensity_dB) {
	LOOP {
		iam (Sound);
		double intensity = Sound_getIntensity_dB (me);
		Melder_informationReal (intensity, U"dB");
	}
END2 }

FORM3 (REAL_Sound_getMaximum, U"Sound: Get maximum", U"Sound: Get maximum...") {
	REAL (U"left Time range (s)", U"0.0")
	REAL (U"right Time range (s)", U"0.0 (= all)")
	RADIO (U"Interpolation", 4)
		RADIOBUTTON (U"None")
		RADIOBUTTON (U"Parabolic")
		RADIOBUTTON (U"Cubic")
		RADIOBUTTON (U"Sinc70")
		RADIOBUTTON (U"Sinc700")
	OK2
DO
	LOOP {
		iam (Sound);
		double maximum = Vector_getMaximum (me, GET_REAL (U"left Time range"), GET_REAL (U"right Time range"), GET_INTEGER (U"Interpolation") - 1);
		Melder_informationReal (maximum, U"Pascal");
	}
END2 }

FORM3 (REAL_old_Sound_getMean, U"Sound: Get mean", U"Sound: Get mean...") {
	REAL (U"left Time range (s)", U"0.0")
	REAL (U"right Time range (s)", U"0.0 (= all)")
	OK2
DO
	LOOP {
		iam (Sound);
		double mean = Vector_getMean (me, GET_REAL (U"left Time range"), GET_REAL (U"right Time range"), Vector_CHANNEL_AVERAGE);
		Melder_informationReal (mean, U"Pascal");
	}
END2 }

FORM3 (REAL_Sound_getMean, U"Sound: Get mean", U"Sound: Get mean...") {
	CHANNEL (U"Channel", U"0 (= all)")
	REAL (U"left Time range (s)", U"0.0")
	REAL (U"right Time range (s)", U"0.0 (= all)")
	OK2
DO_ALTERNATIVE3 (REAL_old_Sound_getMean)
	LOOP {
		iam (Sound);
		long channel = GET_INTEGER (U"Channel");
		if (channel > my ny) channel = 1;
		double mean = Vector_getMean (me, GET_REAL (U"left Time range"), GET_REAL (U"right Time range"), channel);
		Melder_informationReal (mean, U"Pascal");
	}
END2 }

FORM3 (REAL_Sound_getMinimum, U"Sound: Get minimum", U"Sound: Get minimum...") {
	REAL (U"left Time range (s)", U"0.0")
	REAL (U"right Time range (s)", U"0.0 (= all)")
	RADIO (U"Interpolation", 4)
		RADIOBUTTON (U"None")
		RADIOBUTTON (U"Parabolic")
		RADIOBUTTON (U"Cubic")
		RADIOBUTTON (U"Sinc70")
		RADIOBUTTON (U"Sinc700")
	OK2
DO
	LOOP {
		iam (Sound);
		double minimum = Vector_getMinimum (me, GET_REAL (U"left Time range"), GET_REAL (U"right Time range"), GET_INTEGER (U"Interpolation") - 1);
		Melder_informationReal (minimum, U"Pascal");
	}
END2 }

FORM3 (REAL_old_Sound_getNearestZeroCrossing, U"Sound: Get nearest zero crossing", U"Sound: Get nearest zero crossing...") {
	REAL (U"Time (s)", U"0.5")
	OK2
DO
	LOOP {
		iam (Sound);
		if (my ny > 1) Melder_throw (U"Cannot determine a zero crossing for a stereo sound.");
		double zeroCrossing = Sound_getNearestZeroCrossing (me, GET_REAL (U"Time"), 1);
		Melder_informationReal (zeroCrossing, U"seconds");
	}
END2 }

FORM3 (REAL_Sound_getNearestZeroCrossing, U"Sound: Get nearest zero crossing", U"Sound: Get nearest zero crossing...") {
	CHANNEL (U"Channel (number, Left, or Right)", U"1")
	REAL (U"Time (s)", U"0.5")
	OK2
DO_ALTERNATIVE3 (REAL_old_Sound_getNearestZeroCrossing)
	LOOP {
		iam (Sound);
		long channel = GET_INTEGER (U"Channel");
		if (channel > my ny) channel = 1;
		double zeroCrossing = Sound_getNearestZeroCrossing (me, GET_REAL (U"Time"), channel);
		Melder_informationReal (zeroCrossing, U"seconds");
	}
END2 }

DIRECT3 (INTEGER_Sound_getNumberOfChannels) {
	LOOP {
		iam (Sound);
		long numberOfChannels = my ny;
		Melder_information (numberOfChannels, numberOfChannels == 1 ? U" channel (mono)" : numberOfChannels == 2 ? U" channels (stereo)" : U"channels");
	}
END2 }

DIRECT3 (INTEGER_Sound_getNumberOfSamples) {
	LOOP {
		iam (Sound);
		long numberOfSamples = my nx;
		Melder_information (numberOfSamples, U" samples");
	}
END2 }

FORM3 (REAL_Sound_getPower, U"Sound: Get power", U"Sound: Get power...") {
	REAL (U"left Time range (s)", U"0.0")
	REAL (U"right Time range (s)", U"0.0 (= all)")
	OK2
DO
	LOOP {
		iam (Sound);
		double power = Sound_getPower (me, GET_REAL (U"left Time range"), GET_REAL (U"right Time range"));
		Melder_informationReal (power, U"Pa2");
	}
END2 }

DIRECT3 (REAL_Sound_getPowerInAir) {
	LOOP {
		iam (Sound);
		double powerInAir = Sound_getPowerInAir (me);
		Melder_informationReal (powerInAir, U"Watt/m2");
	}
END2 }

FORM3 (REAL_Sound_getRootMeanSquare, U"Sound: Get root-mean-square", U"Sound: Get root-mean-square...") {
	REAL (U"left Time range (s)", U"0.0")
	REAL (U"right Time range (s)", U"0.0 (= all)")
	OK2
DO
	LOOP {
		iam (Sound);
		double rootMeanSquare = Sound_getRootMeanSquare (me, GET_REAL (U"left Time range"), GET_REAL (U"right Time range"));
		Melder_informationReal (rootMeanSquare, U"Pascal");
	}
END2 }

DIRECT3 (REAL_Sound_getSamplePeriod) {
	LOOP {
		iam (Sound);
		double samplePeriod = my dx;
		Melder_informationReal (samplePeriod, U"seconds");
	}
END2 }

DIRECT3 (REAL_Sound_getSampleRate) {
	LOOP {
		iam (Sound);
		double samplingFrequency = 1.0 / my dx;
		Melder_informationReal (samplingFrequency, U"Hz");
	}
END2 }

FORM3 (REAL_old_Sound_getStandardDeviation, U"Sound: Get standard deviation", U"Sound: Get standard deviation...") {
	REAL (U"left Time range (s)", U"0.0")
	REAL (U"right Time range (s)", U"0.0 (= all)")
	OK2
DO
	LOOP {
		iam (Sound);
		double stdev = Vector_getStandardDeviation (me, GET_REAL (U"left Time range"), GET_REAL (U"right Time range"), Vector_CHANNEL_AVERAGE);
		Melder_informationReal (stdev, U"Pascal");
	}
END2 }

FORM3 (REAL_Sound_getStandardDeviation, U"Sound: Get standard deviation", U"Sound: Get standard deviation...") {
	CHANNEL (U"Channel", U"0 (= average)")
	REAL (U"left Time range (s)", U"0.0")
	REAL (U"right Time range (s)", U"0.0 (= all)")
	OK2
DO_ALTERNATIVE3 (REAL_old_Sound_getStandardDeviation)
	LOOP {
		iam (Sound);
		long channel = GET_INTEGER (U"Channel");
		if (channel > my ny) channel = 1;
		double stdev = Vector_getStandardDeviation (me, GET_REAL (U"left Time range"), GET_REAL (U"right Time range"), channel);
		Melder_informationReal (stdev, U"Pascal");
	}
END2 }

FORM3 (REAL_Sound_getTimeFromIndex, U"Get time from sample number", U"Get time from sample number...") {
	INTEGER (U"Sample number", U"100")
	OK2
DO
	LOOP {
		iam (Sound);
		double time = Sampled_indexToX (me, GET_INTEGER (U"Sample number"));
		Melder_informationReal (time, U"seconds");
	}
END2 }

FORM3 (REAL_Sound_getTimeOfMaximum, U"Sound: Get time of maximum", U"Sound: Get time of maximum...") {
	REAL (U"left Time range (s)", U"0.0")
	REAL (U"right Time range (s)", U"0.0 (= all)")
	RADIO (U"Interpolation", 4)
		RADIOBUTTON (U"None")
		RADIOBUTTON (U"Parabolic")
		RADIOBUTTON (U"Cubic")
		RADIOBUTTON (U"Sinc70")
		RADIOBUTTON (U"Sinc700")
	OK2
DO
	LOOP {
		iam (Sound);
		double time = Vector_getXOfMaximum (me, GET_REAL (U"left Time range"), GET_REAL (U"right Time range"), GET_INTEGER (U"Interpolation") - 1);
		Melder_informationReal (time, U"seconds");
	}
END2 }

FORM3 (REAL_Sound_getTimeOfMinimum, U"Sound: Get time of minimum", U"Sound: Get time of minimum...") {
	REAL (U"left Time range (s)", U"0.0")
	REAL (U"right Time range (s)", U"0.0 (= all)")
	RADIO (U"Interpolation", 4)
		RADIOBUTTON (U"None")
		RADIOBUTTON (U"Parabolic")
		RADIOBUTTON (U"Cubic")
		RADIOBUTTON (U"Sinc70")
		RADIOBUTTON (U"Sinc700")
	OK2
DO
	LOOP {
		iam (Sound);
		double time = Vector_getXOfMinimum (me, GET_REAL (U"left Time range"), GET_REAL (U"right Time range"), GET_INTEGER (U"Interpolation") - 1);
		Melder_informationReal (time, U"seconds");
	}
END2 }

FORM3 (REAL_old_Sound_getValueAtIndex, U"Sound: Get value at sample number", U"Sound: Get value at sample number...") {
	INTEGER (U"Sample number", U"100")
	OK2
DO
	LOOP {
		iam (Sound);
		long sampleIndex = GET_INTEGER (U"Sample number");
		Melder_informationReal (sampleIndex < 1 || sampleIndex > my nx ? NUMundefined :
			my ny == 1 ? my z [1] [sampleIndex] : 0.5 * (my z [1] [sampleIndex] + my z [2] [sampleIndex]), U"Pascal");
	}
END2 }

FORM3 (REAL_Sound_getValueAtIndex, U"Sound: Get value at sample number", U"Sound: Get value at sample number...") {
	CHANNEL (U"Channel", U"0 (= average)")
	INTEGER (U"Sample number", U"100")
	OK2
DO_ALTERNATIVE3 (REAL_old_Sound_getValueAtIndex)
	LOOP {
		iam (Sound);
		long sampleIndex = GET_INTEGER (U"Sample number");
		long channel = GET_INTEGER (U"Channel");
		if (channel > my ny) channel = 1;
		Melder_informationReal (sampleIndex < 1 || sampleIndex > my nx ? NUMundefined :
			Sampled_getValueAtSample (me, sampleIndex, channel, 0), U"Pascal");
	}
END2 }

FORM3 (REAL_old_Sound_getValueAtTime, U"Sound: Get value at time", U"Sound: Get value at time...") {
	REAL (U"Time (s)", U"0.5")
	RADIO (U"Interpolation", 4)
		RADIOBUTTON (U"Nearest")
		RADIOBUTTON (U"Linear")
		RADIOBUTTON (U"Cubic")
		RADIOBUTTON (U"Sinc70")
		RADIOBUTTON (U"Sinc700")
	OK2
DO
	LOOP {
		iam (Sound);
		double value = Vector_getValueAtX (me, GET_REAL (U"Time"), Vector_CHANNEL_AVERAGE, GET_INTEGER (U"Interpolation") - 1);
		Melder_informationReal (value, U"Pascal");
	}
END2 }

FORM3 (REAL_Sound_getValueAtTime, U"Sound: Get value at time", U"Sound: Get value at time...") {
	CHANNEL (U"Channel", U"0 (= average)")
	REAL (U"Time (s)", U"0.5")
	RADIO (U"Interpolation", 4)
		RADIOBUTTON (U"Nearest")
		RADIOBUTTON (U"Linear")
		RADIOBUTTON (U"Cubic")
		RADIOBUTTON (U"Sinc70")
		RADIOBUTTON (U"Sinc700")
	OK2
DO_ALTERNATIVE3 (REAL_old_Sound_getValueAtTime)
	LOOP {
		iam (Sound);
		long channel = GET_INTEGER (U"Channel");
		if (channel > my ny) channel = 1;
		double value = Vector_getValueAtX (me, GET_REAL (U"Time"), channel, GET_INTEGER (U"Interpolation") - 1);
		Melder_informationReal (value, U"Pascal");
	}
END2 }

DIRECT3 (HELP_Sound_help) {
	Melder_help (U"Sound");
END2 }

FORM3 (NEW_Sound_lengthen_overlapAdd, U"Sound: Lengthen (overlap-add)", U"Sound: Lengthen (overlap-add)...") {
	POSITIVE (U"Minimum pitch (Hz)", U"75")
	POSITIVE (U"Maximum pitch (Hz)", U"600")
	POSITIVE (U"Factor", U"1.5")
	OK2
DO
	double minimumPitch = GET_REAL (U"Minimum pitch"), maximumPitch = GET_REAL (U"Maximum pitch");
	double factor = GET_REAL (U"Factor");
	if (minimumPitch >= maximumPitch) Melder_throw (U"Maximum pitch should be greater than minimum pitch.");
	LOOP {
		iam (Sound);
		autoSound thee = Sound_lengthen_overlapAdd (me, minimumPitch, maximumPitch, factor);
		praat_new (thee.move(), my name, U"_", Melder_fixed (factor, 2));
	}
END2 }

FORM3 (MODIFY_Sound_multiply, U"Sound: Multiply", nullptr) {
	REAL (U"Multiplication factor", U"1.5")
	OK2
DO
	LOOP {
		iam (Sound);
		Vector_multiplyByScalar (me, GET_REAL (U"Multiplication factor"));
		praat_dataChanged (me);
	}
END2 }

FORM3 (MODIFY_Sound_multiplyByWindow, U"Sound: Multiply by window", nullptr) {
	OPTIONMENU_ENUM (U"Window shape", kSound_windowShape, HANNING)
	OK2
DO
	LOOP {
		iam (Sound);
		Sound_multiplyByWindow (me, GET_ENUM (kSound_windowShape, U"Window shape"));
		praat_dataChanged (me);
	}
END2 }

FORM3 (MODIFY_Sound_overrideSamplingFrequency, U"Sound: Override sampling frequency", nullptr) {
	POSITIVE (U"New sampling frequency (Hz)", U"16000.0")
	OK2
DO
	LOOP {
		iam (Sound);
		Sound_overrideSamplingFrequency (me, GET_REAL (U"New sampling frequency"));
		praat_dataChanged (me);
	}
END2 }

DIRECT3 (PLAY_Sound_play) {
	int n = 0;
	LOOP {
		n ++;
	}
	if (n == 1 || MelderAudio_getOutputMaximumAsynchronicity () < kMelder_asynchronicityLevel_ASYNCHRONOUS) {
		LOOP {
			iam (Sound);
			Sound_play (me, nullptr, nullptr);
		}
	} else {
		MelderAudio_setOutputMaximumAsynchronicity (kMelder_asynchronicityLevel_INTERRUPTABLE);
		LOOP {
			iam (Sound);
			Sound_play (me, nullptr, nullptr);   // BUG: exception-safe?
		}
		MelderAudio_setOutputMaximumAsynchronicity (kMelder_asynchronicityLevel_ASYNCHRONOUS);
	}
END2 }

FORM3 (MODIFY_Sound_preemphasizeInline, U"Sound: Pre-emphasize (in-line)", U"Sound: Pre-emphasize (in-line)...") {
	REAL (U"From frequency (Hz)", U"50.0")
	OK2
DO
	LOOP {
		iam (Sound);
		Sound_preEmphasis (me, GET_REAL (U"From frequency"));
		Vector_scale (me, 0.99);
		praat_dataChanged (me);
	}
END2 }

FORM_READ3 (READMANY_Sound_readSeparateChannelsFromSoundFile, U"Read separate channels from sound file", nullptr, true) {
	autoSound sound = Sound_readFromSoundFile (file);
	char32 name [300];
	Melder_sprint (name,300, MelderFile_name (file));
	char32 *lastPeriod = str32rchr (name, U'.');
	if (lastPeriod) {
		*lastPeriod = U'\0';
	}
	for (long ichan = 1; ichan <= sound -> ny; ichan ++) {
		autoSound thee = Sound_extractChannel (sound.get(), ichan);
		praat_new (thee.move(), name, U"_ch", ichan);
	}
END2 }

FORM_READ3 (READ1_Sound_readFromRawAlawFile, U"Read Sound from raw Alaw file", nullptr, true) {
	autoSound me = Sound_readFromRawAlawFile (file);
	praat_new (me.move(), MelderFile_name (file));
END2 }

static SoundRecorder theReferenceToTheOnlySoundRecorder;
static int thePreviousNumberOfChannels;

static void cb_SoundRecorder_destruction (SoundRecorder /* me */) {
	theReferenceToTheOnlySoundRecorder = nullptr;
}
static void cb_SoundRecorder_publication (Editor /* me */, autoDaata publication) {
	try {
		praat_new (publication.move());
	} catch (MelderError) {
		Melder_flushError ();
	}
	praat_updateSelection ();
}
static void do_Sound_record (int numberOfChannels) {
	if (theCurrentPraatApplication -> batch)
		Melder_throw (U"Cannot record a Sound from batch.");
	if (theReferenceToTheOnlySoundRecorder && numberOfChannels == thePreviousNumberOfChannels) {
		Editor_raise (theReferenceToTheOnlySoundRecorder);
	} else {
		forget (theReferenceToTheOnlySoundRecorder);
		autoSoundRecorder recorder = SoundRecorder_create (numberOfChannels);
		Editor_setDestructionCallback (recorder.get(), cb_SoundRecorder_destruction);
		Editor_setPublicationCallback (recorder.get(), cb_SoundRecorder_publication);
		theReferenceToTheOnlySoundRecorder = recorder.get();
		recorder.releaseToUser();
		thePreviousNumberOfChannels = numberOfChannels;
	}
}
DIRECT3 (WINDOW_Sound_recordMono) {
	do_Sound_record (1);
END2 }
DIRECT3 (WINDOW_Sound_recordStereo) {
	do_Sound_record (2);
END2 }

FORM3 (RECORD1_Sound_record_fixedTime, U"Record Sound", nullptr) {
	RADIO (U"Input source", 1)
		RADIOBUTTON (U"Microphone")
		RADIOBUTTON (U"Line")
	REAL (U"Gain (0-1)", U"0.1")
	REAL (U"Balance (0-1)", U"0.5")
	RADIO (U"Sampling frequency", 1)
		#ifdef UNIX
		RADIOBUTTON (U"8000")
		#endif
		#ifndef macintosh
		RADIOBUTTON (U"11025")
		#endif
		#ifdef UNIX
		RADIOBUTTON (U"16000")
		#endif
		#ifndef macintosh
		RADIOBUTTON (U"22050")
		#endif
		#ifdef UNIX
		RADIOBUTTON (U"32000")
		#endif
		RADIOBUTTON (U"44100")
		RADIOBUTTON (U"48000")
		RADIOBUTTON (U"96000")
	POSITIVE (U"Duration (seconds)", U"1.0")
	OK2
DO
	autoSound me = Sound_record_fixedTime (GET_INTEGER (U"Input source"),
		GET_REAL (U"Gain"), GET_REAL (U"Balance"),
		Melder_atof (GET_STRING (U"Sampling frequency")), GET_REAL (U"Duration"));
	praat_new (me.move(), U"untitled");
END2 }

extern "C" void* Praat_Sound_resample (void* sound, double newSamplingFrequency, int precision);
#if 0
void* Praat_Sound_resample (void* sound, double newSamplingFrequency, int precision) {
	try {
		if (newSamplingFrequency <= 0.0) Melder_throw (U"`newSamplingFrequency` has to be positive.");
		if (precision <= 0) Melder_throw (U"`precision` has to be greater than 0.");
		autoSound thee = Sound_resample ((Sound) sound, newSamplingFrequency, precision);
		return (void*) thee.releaseToAmbiguousOwner();
	} catch (MelderError) {
		Melder_flushError (U"Praat: Sound_resample: not performed.");
		return NULL;
	}
}
#elif 0
typedef struct {
	int type;
	union { void* _object; double _double; int _int; };
} PraatLibArg;
#define PraatLibArg_OBJECT  0
#define PraatLibArg_DOUBLE  1
#define PraatLibArg_INT  2
extern "C" void* praatlib_do (const char *commandTitle, PraatLibArg args [], int narg);
void* Praat_Sound_resample (void* sound, double newSamplingFrequency, int precision) {
	PraatLibArg args [3];
	args [0]. type = PraatLibArg_OBJECT;
	args [0]. _object = sound;
	args [1]. type = PraatLibArg_DOUBLE;
	args [1]. _double = newSamplingFrequency;
	args [2]. type = PraatLibArg_INT;
	args [2]. _int = precision;
	return praatlib_do ("Sound: Resample...", args, 3);
}
#endif

FORM3 (NEW_Sound_resample, U"Sound: Resample", U"Sound: Resample...") {
	POSITIVE (U"New sampling frequency (Hz)", U"10000.0")
	NATURAL (U"Precision (samples)", U"50")
	OK2
DO
	double samplingFrequency = GET_REAL (U"New sampling frequency");
	LOOP {
		iam (Sound);
		autoSound thee = Sound_resample (me, samplingFrequency, GET_INTEGER (U"Precision"));
		praat_new (thee.move(), my name, U"_", (long) round (samplingFrequency));
	}
END2 }

DIRECT3 (MODIFY_Sound_reverse) {
	LOOP {
		iam (Sound);
		Sound_reverse (me, 0.0, 0.0);
		praat_dataChanged (me);
	}
END2 }

FORM_WRITE3 (SAVE_Sound_saveAs24BitWavFile, U"Save as 24-bit WAV file", nullptr, U"wav") {
	autoSoundAndLongSoundList list = SoundAndLongSoundList_create ();
	LOOP {
		iam (Sampled);
		list -> addItem_ref (me);
	}
	LongSound_concatenate (list.get(), file, Melder_WAV, 24);
END2 }

FORM_WRITE3 (SAVE_Sound_saveAs32BitWavFile, U"Save as 32-bit WAV file", nullptr, U"wav") {
	autoSoundAndLongSoundList list = SoundAndLongSoundList_create ();
	LOOP {
		iam (Sampled);
		list -> addItem_ref (me);
	}
	LongSound_concatenate (list.get(), file, Melder_WAV, 32);
END2 }

FORM3 (MODIFY_Sound_scalePeak, U"Sound: Scale peak", U"Sound: Scale peak...") {
	POSITIVE (U"New absolute peak", U"0.99")
	OK2
DO
	LOOP {
		iam (Sound);
		Vector_scale (me, GET_REAL (U"New absolute peak"));
		praat_dataChanged (me);
	}
END2 }

FORM3 (MODIFY_Sound_scaleIntensity, U"Sound: Scale intensity", U"Sound: Scale intensity...") {
	POSITIVE (U"New average intensity (dB SPL)", U"70.0")
	OK2
DO
	LOOP {
		iam (Sound);
		Sound_scaleIntensity (me, GET_REAL (U"New average intensity"));
		praat_dataChanged (me);
	}
END2 }

FORM3 (MODIFY_old_Sound_setValueAtIndex, U"Sound: Set value at sample number", U"Sound: Set value at sample number...") {
	NATURAL (U"Sample number", U"100")
	REAL (U"New value", U"0.0")
	OK2
DO
	LOOP {
		iam (Sound);
		long index = GET_INTEGER (U"Sample number");
		if (index > my nx)
			Melder_throw (U"The sample number should not exceed the number of samples, which is ", my nx, U".");
		for (long channel = 1; channel <= my ny; channel ++)
			my z [channel] [index] = GET_REAL (U"New value");
		praat_dataChanged (me);
	}
END2 }

FORM3 (MODIFY_Sound_setValueAtIndex, U"Sound: Set value at sample number", U"Sound: Set value at sample number...") {
	CHANNEL (U"Channel", U"0 (= all)")
	NATURAL (U"Sample number", U"100")
	REAL (U"New value", U"0.0")
	OK2
DO_ALTERNATIVE3 (MODIFY_old_Sound_setValueAtIndex)
	LOOP {
		iam (Sound);
		long index = GET_INTEGER (U"Sample number");
		if (index > my nx)
			Melder_throw (U"The sample number should not exceed the number of samples, which is ", my nx, U".");
		long channel = GET_INTEGER (U"Channel");
		if (channel > my ny) channel = 1;
		if (channel > 0) {
			my z [channel] [index] = GET_REAL (U"New value");
		} else {
			for (channel = 1; channel <= my ny; channel ++) {
				my z [channel] [index] = GET_REAL (U"New value");
			}
		}
		praat_dataChanged (me);
	}
END2 }

FORM3 (MODIFY_Sound_setPartToZero, U"Sound: Set part to zero", nullptr) {
	REAL (U"left Time range (s)", U"0.0")
	REAL (U"right Time range (s)", U"0.0 (= all)")
	RADIO (U"Cut", 2)
		OPTION (U"at exactly these times")
		OPTION (U"at nearest zero crossing")
	OK2
DO
	LOOP {
		iam (Sound);
		Sound_setZero (me, GET_REAL (U"left Time range"), GET_REAL (U"right Time range"), GET_INTEGER (U"Cut") - 1);
		praat_dataChanged (me);
	}
END2 }

DIRECT3 (MODIFY_Sound_subtractMean) {
	LOOP {
		iam (Sound);
		Vector_subtractMean (me);
		praat_dataChanged (me);
	}
END2 }

FORM3 (NEW_Sound_to_Manipulation, U"Sound: To Manipulation", U"Manipulation") {
	POSITIVE (U"Time step (s)", U"0.01")
	POSITIVE (U"Minimum pitch (Hz)", U"75.0")
	POSITIVE (U"Maximum pitch (Hz)", U"600.0")
	OK2
DO
	double fmin = GET_REAL (U"Minimum pitch"), fmax = GET_REAL (U"Maximum pitch");
	if (fmax <= fmin) Melder_throw (U"Maximum pitch must be greater than minimum pitch.");
	LOOP {
		iam (Sound);
		autoManipulation thee = Sound_to_Manipulation (me, GET_REAL (U"Time step"), fmin, fmax);
		praat_new (thee.move(), my name);
	}
END2 }

FORM3 (NEW_Sound_to_Cochleagram, U"Sound: To Cochleagram", nullptr) {
	POSITIVE (U"Time step (s)", U"0.01")
	POSITIVE (U"Frequency resolution (Bark)", U"0.1")
	POSITIVE (U"Window length (s)", U"0.03")
	REAL (U"Forward-masking time (s)", U"0.03")
	OK2
DO
	LOOP {
		iam (Sound);
		autoCochleagram thee = Sound_to_Cochleagram (me, GET_REAL (U"Time step"),
			GET_REAL (U"Frequency resolution"), GET_REAL (U"Window length"), GET_REAL (U"Forward-masking time"));
		praat_new (thee.move(), my name);
	}
END2 }

FORM3 (NEW_Sound_to_Cochleagram_edb, U"Sound: To Cochleagram (De Boer, Meddis & Hewitt)", nullptr) {
	POSITIVE (U"Time step (s)", U"0.01")
	POSITIVE (U"Frequency resolution (Bark)", U"0.1")
	BOOLEAN (U"Has synapse", true)
	LABEL (U"", U"Meddis synapse properties")
	POSITIVE (U"   replenishment rate (/sec)", U"5.05")
	POSITIVE (U"   loss rate (/sec)", U"2500.0")
	POSITIVE (U"   return rate (/sec)", U"6580.0")
	POSITIVE (U"   reprocessing rate (/sec)", U"66.31")
	OK2
DO
	LOOP {
		iam (Sound);
		autoCochleagram thee = Sound_to_Cochleagram_edb (me, GET_REAL (U"Time step"),
			GET_REAL (U"Frequency resolution"), GET_INTEGER (U"Has synapse"),
			GET_REAL (U"   replenishment rate"), GET_REAL (U"   loss rate"),
			GET_REAL (U"   return rate"), GET_REAL (U"   reprocessing rate"));
		praat_new (thee.move(), my name);
	}
END2 }

FORM3 (NEW_Sound_to_Formant_burg, U"Sound: To Formant (Burg method)", U"Sound: To Formant (burg)...") {
	REAL (U"Time step (s)", U"0.0 (= auto)")
	POSITIVE (U"Max. number of formants", U"5.0")
	REAL (U"Maximum formant (Hz)", U"5500.0 (= adult female)")
	POSITIVE (U"Window length (s)", U"0.025")
	POSITIVE (U"Pre-emphasis from (Hz)", U"50.0")
	OK2
DO
	LOOP {
		iam (Sound);
		autoFormant thee = Sound_to_Formant_burg (me, GET_REAL (U"Time step"),
			GET_REAL (U"Max. number of formants"), GET_REAL (U"Maximum formant"),
			GET_REAL (U"Window length"), GET_REAL (U"Pre-emphasis from"));
		praat_new (thee.move(), my name);
	}
END2 }

FORM3 (NEW_Sound_to_Formant_keepAll, U"Sound: To Formant (keep all)", U"Sound: To Formant (keep all)...") {
	REAL (U"Time step (s)", U"0.0 (= auto)")
	POSITIVE (U"Max. number of formants", U"5.0")
	REAL (U"Maximum formant (Hz)", U"5500.0 (= adult female)")
	POSITIVE (U"Window length (s)", U"0.025")
	POSITIVE (U"Pre-emphasis from (Hz)", U"50.0")
	OK2
DO
	LOOP {
		iam (Sound);
		autoFormant thee = Sound_to_Formant_keepAll (me, GET_REAL (U"Time step"),
			GET_REAL (U"Max. number of formants"), GET_REAL (U"Maximum formant"),
			GET_REAL (U"Window length"), GET_REAL (U"Pre-emphasis from"));
		praat_new (thee.move(), my name);
	}
END2 }

FORM3 (NEW_Sound_to_Formant_willems, U"Sound: To Formant (split Levinson (Willems))", U"Sound: To Formant (sl)...") {
	REAL (U"Time step (s)", U"0.0 (= auto)")
	POSITIVE (U"Number of formants", U"5.0")
	REAL (U"Maximum formant (Hz)", U"5500.0 (= adult female)")
	POSITIVE (U"Window length (s)", U"0.025")
	POSITIVE (U"Pre-emphasis from (Hz)", U"50.0")
	OK2
DO
	LOOP {
		iam (Sound);
		autoFormant thee = Sound_to_Formant_willems (me, GET_REAL (U"Time step"),
			GET_REAL (U"Number of formants"), GET_REAL (U"Maximum formant"),
			GET_REAL (U"Window length"), GET_REAL (U"Pre-emphasis from"));
		praat_new (thee.move(), my name);
	}
END2 }

FORM3 (NEW_Sound_to_Harmonicity_ac, U"Sound: To Harmonicity (ac)", U"Sound: To Harmonicity (ac)...") {
	POSITIVE (U"Time step (s)", U"0.01")
	POSITIVE (U"Minimum pitch (Hz)", U"75.0")
	REAL (U"Silence threshold", U"0.1")
	POSITIVE (U"Periods per window", U"4.5")
	OK2
DO
	double periodsPerWindow = GET_REAL (U"Periods per window");
	if (periodsPerWindow < 3.0) Melder_throw (U"Number of periods per window must be at least 3.0.");
	LOOP {
		iam (Sound);
		autoHarmonicity thee = Sound_to_Harmonicity_ac (me, GET_REAL (U"Time step"),
			GET_REAL (U"Minimum pitch"), GET_REAL (U"Silence threshold"), periodsPerWindow);
		praat_new (thee.move(), my name);
	}
END2 }

FORM3 (NEW_Sound_to_Harmonicity_cc, U"Sound: To Harmonicity (cc)", U"Sound: To Harmonicity (cc)...") {
	POSITIVE (U"Time step (s)", U"0.01")
	POSITIVE (U"Minimum pitch (Hz)", U"75.0")
	REAL (U"Silence threshold", U"0.1")
	POSITIVE (U"Periods per window", U"1.0")
	OK2
DO
	LOOP {
		iam (Sound);
		autoHarmonicity thee = Sound_to_Harmonicity_cc (me, GET_REAL (U"Time step"),
			GET_REAL (U"Minimum pitch"), GET_REAL (U"Silence threshold"),
			GET_REAL (U"Periods per window"));
		praat_new (thee.move(), my name);
	}
END2 }

FORM3 (NEW_Sound_to_Harmonicity_gne, U"Sound: To Harmonicity (gne)", nullptr) {
	POSITIVE (U"Minimum frequency (Hz)", U"500.0")
	POSITIVE (U"Maximum frequency (Hz)", U"4500.0")
	POSITIVE (U"Bandwidth (Hz)", U"1000.0")
	POSITIVE (U"Step (Hz)", U"80.0")
	OK2
DO
	LOOP {
		iam (Sound);
		autoMatrix thee = Sound_to_Harmonicity_GNE (me, GET_REAL (U"Minimum frequency"),
			GET_REAL (U"Maximum frequency"), GET_REAL (U"Bandwidth"),
			GET_REAL (U"Step"));
		praat_new (thee.move(), my name);
	}
END2 }

FORM3 (NEW_old_Sound_to_Intensity, U"Sound: To Intensity", U"Sound: To Intensity...") {
	POSITIVE (U"Minimum pitch (Hz)", U"100.0")
	REAL (U"Time step (s)", U"0.0 (= auto)")
	OK2
DO
	LOOP {
		iam (Sound);
		autoIntensity thee = Sound_to_Intensity (me,
			GET_REAL (U"Minimum pitch"), GET_REAL (U"Time step"), false);
		praat_new (thee.move(), my name);
	}
END2 }

FORM3 (NEW_Sound_to_Intensity, U"Sound: To Intensity", U"Sound: To Intensity...") {
	POSITIVE (U"Minimum pitch (Hz)", U"100.0")
	REAL (U"Time step (s)", U"0.0 (= auto)")
	BOOLEAN (U"Subtract mean", true)
	OK2
DO_ALTERNATIVE3 (NEW_old_Sound_to_Intensity)
	LOOP {
		iam (Sound);
		autoIntensity thee = Sound_to_Intensity (me,
			GET_REAL (U"Minimum pitch"), GET_REAL (U"Time step"), GET_INTEGER (U"Subtract mean"));
		praat_new (thee.move(), my name);
	}
END2 }

FORM3 (NEW_Sound_to_IntensityTier, U"Sound: To IntensityTier", nullptr) {
	POSITIVE (U"Minimum pitch (Hz)", U"100.0")
	REAL (U"Time step (s)", U"0.0 (= auto)")
	BOOLEAN (U"Subtract mean", true)
	OK2
DO
	LOOP {
		iam (Sound);
		autoIntensityTier thee = Sound_to_IntensityTier (me,
			GET_REAL (U"Minimum pitch"), GET_REAL (U"Time step"), GET_INTEGER (U"Subtract mean"));
		praat_new (thee.move(), my name);
	}
END2 }

DIRECT3 (NEW_Sound_to_IntervalTier) {
	LOOP {
		iam (Sound);
		autoIntervalTier thee = IntervalTier_create (my xmin, my xmax);
		praat_new (thee.move(), my name);
	}
END2 }

FORM3 (NEW_Sound_to_Ltas, U"Sound: To long-term average spectrum", nullptr) {
	POSITIVE (U"Bandwidth (Hz)", U"100")
	OK2
DO
	LOOP {
		iam (Sound);
		autoLtas thee = Sound_to_Ltas (me, GET_REAL (U"Bandwidth"));
		praat_new (thee.move(), my name);
	}
END2 }

FORM3 (NEW_Sound_to_Ltas_pitchCorrected, U"Sound: To Ltas (pitch-corrected)", U"Sound: To Ltas (pitch-corrected)...") {
	POSITIVE (U"Minimum pitch (Hz)", U"75.0")
	POSITIVE (U"Maximum pitch (Hz)", U"600.0")
	POSITIVE (U"Maximum frequency (Hz)", U"5000.0")
	POSITIVE (U"Bandwidth (Hz)", U"100.0")
	REAL (U"Shortest period (s)", U"0.0001")
	REAL (U"Longest period (s)", U"0.02")
	POSITIVE (U"Maximum period factor", U"1.3")
	OK2
DO
	double fmin = GET_REAL (U"Minimum pitch"), fmax = GET_REAL (U"Maximum pitch");
	if (fmax <= fmin) Melder_throw (U"Maximum pitch must be greater than minimum pitch.");
	LOOP {
		iam (Sound);
		autoLtas thee = Sound_to_Ltas_pitchCorrected (me, fmin, fmax,
			GET_REAL (U"Maximum frequency"), GET_REAL (U"Bandwidth"),
			GET_REAL (U"Shortest period"), GET_REAL (U"Longest period"), GET_REAL (U"Maximum period factor"));
		praat_new (thee.move(), my name);
	}
END2 }

DIRECT3 (NEW_Sound_to_Matrix) {
	LOOP {
		iam (Sound);
		autoMatrix thee = Sound_to_Matrix (me);
		praat_new (thee.move(), my name);
	}
END2 }

DIRECT3 (NEW1_Sounds_to_ParamCurve) {
	Sound s1 = nullptr, s2 = nullptr;
	LOOP {
		iam (Sound);
		( s1 ? s2 : s1 ) = me;
	}
	autoParamCurve thee = ParamCurve_create (s1, s2);
	praat_new (thee.move(), s1 -> name, U"_", s2 -> name);
END2 }

FORM3 (NEW_Sound_to_Pitch, U"Sound: To Pitch", U"Sound: To Pitch...") {
	REAL (U"Time step (s)", U"0.0 (= auto)")
	POSITIVE (U"Pitch floor (Hz)", U"75.0")
	POSITIVE (U"Pitch ceiling (Hz)", U"600.0")
	OK2
DO
	LOOP {
		iam (Sound);
		autoPitch thee = Sound_to_Pitch (me, GET_REAL (U"Time step"), GET_REAL (U"Pitch floor"), GET_REAL (U"Pitch ceiling"));
		praat_new (thee.move(), my name);
	}
END2 }

FORM3 (NEW_Sound_to_Pitch_ac, U"Sound: To Pitch (ac)", U"Sound: To Pitch (ac)...") {
	LABEL (U"", U"Finding the candidates")
	REAL (U"Time step (s)", U"0.0 (= auto)")
	POSITIVE (U"Pitch floor (Hz)", U"75.0")
	NATURAL (U"Max. number of candidates", U"15")
	BOOLEAN (U"Very accurate", false)
	LABEL (U"", U"Finding a path")
	REAL (U"Silence threshold", U"0.03")
	REAL (U"Voicing threshold", U"0.45")
	REAL (U"Octave cost", U"0.01")
	REAL (U"Octave-jump cost", U"0.35")
	REAL (U"Voiced / unvoiced cost", U"0.14")
	POSITIVE (U"Pitch ceiling (Hz)", U"600.0")
	OK2
DO
	long maxnCandidates = GET_INTEGER (U"Max. number of candidates");
	if (maxnCandidates <= 1) Melder_throw (U"Maximum number of candidates must be greater than 1.");
	LOOP {
		iam (Sound);
		autoPitch thee = Sound_to_Pitch_ac (me, GET_REAL (U"Time step"),
			GET_REAL (U"Pitch floor"), 3.0, maxnCandidates, GET_INTEGER (U"Very accurate"),
			GET_REAL (U"Silence threshold"), GET_REAL (U"Voicing threshold"),
			GET_REAL (U"Octave cost"), GET_REAL (U"Octave-jump cost"),
			GET_REAL (U"Voiced / unvoiced cost"), GET_REAL (U"Pitch ceiling"));
		praat_new (thee.move(), my name);
	}
END2 }

FORM3 (NEW_Sound_to_Pitch_cc, U"Sound: To Pitch (cc)", U"Sound: To Pitch (cc)...") {
	LABEL (U"", U"Finding the candidates")
	REAL (U"Time step (s)", U"0.0 (= auto)")
	POSITIVE (U"Pitch floor (Hz)", U"75.0")
	NATURAL (U"Max. number of candidates", U"15")
	BOOLEAN (U"Very accurate", false)
	LABEL (U"", U"Finding a path")
	REAL (U"Silence threshold", U"0.03")
	REAL (U"Voicing threshold", U"0.45")
	REAL (U"Octave cost", U"0.01")
	REAL (U"Octave-jump cost", U"0.35")
	REAL (U"Voiced / unvoiced cost", U"0.14")
	POSITIVE (U"Pitch ceiling (Hz)", U"600")
	OK2
DO
	long maxnCandidates = GET_INTEGER (U"Max. number of candidates");
	if (maxnCandidates <= 1) Melder_throw (U"Maximum number of candidates must be greater than 1.");
	LOOP {
		iam (Sound);
		autoPitch thee = Sound_to_Pitch_cc (me, GET_REAL (U"Time step"),
			GET_REAL (U"Pitch floor"), 1.0, maxnCandidates, GET_INTEGER (U"Very accurate"),
			GET_REAL (U"Silence threshold"), GET_REAL (U"Voicing threshold"),
			GET_REAL (U"Octave cost"), GET_REAL (U"Octave-jump cost"),
			GET_REAL (U"Voiced / unvoiced cost"), GET_REAL (U"Pitch ceiling"));
		praat_new (thee.move(), my name);
	}
END2 }

FORM3 (NEW_Sound_to_PointProcess_extrema, U"Sound: To PointProcess (extrema)", nullptr) {
	CHANNEL (U"Channel (number, Left, or Right)", U"1")
	BOOLEAN (U"Include maxima", true)
	BOOLEAN (U"Include minima", false)
	RADIO (U"Interpolation", 4)
		RADIOBUTTON (U"None")
		RADIOBUTTON (U"Parabolic")
		RADIOBUTTON (U"Cubic")
		RADIOBUTTON (U"Sinc70")
		RADIOBUTTON (U"Sinc700")
	OK2
DO
	long channel = GET_INTEGER (U"Channel");
	LOOP {
		iam (Sound);
		autoPointProcess thee = Sound_to_PointProcess_extrema (me, channel > my ny ? 1 : channel, GET_INTEGER (U"Interpolation") - 1,
			GET_INTEGER (U"Include maxima"), GET_INTEGER (U"Include minima"));
		praat_new (thee.move(), my name);
	}
END2 }

FORM3 (NEW_Sound_to_PointProcess_periodic_cc, U"Sound: To PointProcess (periodic, cc)", U"Sound: To PointProcess (periodic, cc)...") {
	POSITIVE (U"Minimum pitch (Hz)", U"75")
	POSITIVE (U"Maximum pitch (Hz)", U"600")
	OK2
DO
	double fmin = GET_REAL (U"Minimum pitch"), fmax = GET_REAL (U"Maximum pitch");
	if (fmax <= fmin) Melder_throw (U"Maximum pitch must be greater than minimum pitch.");
	LOOP {
		iam (Sound);
		autoPointProcess thee = Sound_to_PointProcess_periodic_cc (me, fmin, fmax);
		praat_new (thee.move(), my name);
	}
END2 }

FORM3 (NEW_Sound_to_PointProcess_periodic_peaks, U"Sound: To PointProcess (periodic, peaks)", U"Sound: To PointProcess (periodic, peaks)...") {
	POSITIVE (U"Minimum pitch (Hz)", U"75")
	POSITIVE (U"Maximum pitch (Hz)", U"600")
	BOOLEAN (U"Include maxima", true)
	BOOLEAN (U"Include minima", false)
	OK2
DO
	double fmin = GET_REAL (U"Minimum pitch"), fmax = GET_REAL (U"Maximum pitch");
	if (fmax <= fmin) Melder_throw (U"Maximum pitch must be greater than minimum pitch.");
	LOOP {
		iam (Sound);
		autoPointProcess thee = Sound_to_PointProcess_periodic_peaks (me, fmin, fmax, GET_INTEGER (U"Include maxima"), GET_INTEGER (U"Include minima"));
		praat_new (thee.move(), my name);
	}
END2 }

FORM3 (NEW_Sound_to_PointProcess_zeroes, U"Get zeroes", nullptr) {
	CHANNEL (U"Channel (number, Left, or Right)", U"1")
	BOOLEAN (U"Include raisers", true)
	BOOLEAN (U"Include fallers", false)
	OK2
DO
	long channel = GET_INTEGER (U"Channel");
	LOOP {
		iam (Sound);
		autoPointProcess thee = Sound_to_PointProcess_zeroes (me, channel > my ny ? 1 : channel, GET_INTEGER (U"Include raisers"), GET_INTEGER (U"Include fallers"));
		praat_new (thee.move(), my name);
	}
END2 }

FORM3 (NEW_Sound_to_Spectrogram, U"Sound: To Spectrogram", U"Sound: To Spectrogram...") {
	POSITIVE (U"Window length (s)", U"0.005")
	POSITIVE (U"Maximum frequency (Hz)", U"5000.0")
	POSITIVE (U"Time step (s)", U"0.002")
	POSITIVE (U"Frequency step (Hz)", U"20.0")
	RADIO_ENUM (U"Window shape", kSound_to_Spectrogram_windowShape, DEFAULT)
	OK2
DO
	LOOP {
		iam (Sound);
		autoSpectrogram thee = Sound_to_Spectrogram (me, GET_REAL (U"Window length"),
			GET_REAL (U"Maximum frequency"), GET_REAL (U"Time step"),
			GET_REAL (U"Frequency step"), GET_ENUM (kSound_to_Spectrogram_windowShape, U"Window shape"), 8.0, 8.0);
		praat_new (thee.move(), my name);
	}
END2 }

FORM3 (NEW_Sound_to_Spectrum, U"Sound: To Spectrum", U"Sound: To Spectrum...") {
	BOOLEAN (U"Fast", true)
	OK2
DO
	LOOP {
		iam (Sound);
		autoSpectrum thee = Sound_to_Spectrum (me, GET_INTEGER (U"Fast"));
		praat_new (thee.move(), my name);
	}
END2 }

DIRECT3 (NEW_Sound_to_Spectrum_dft) {
	LOOP {
		iam (Sound);
		autoSpectrum thee = Sound_to_Spectrum (me, false);
		praat_new (thee.move(), my name);
	}
END2 }

DIRECT3 (NEW_Sound_to_Spectrum_fft) {
	LOOP {
		iam (Sound);
		autoSpectrum thee = Sound_to_Spectrum (me, true);
		praat_new (thee.move(), my name);
	}
END2 }

FORM3 (NEW_Sound_to_TextGrid, U"Sound: To TextGrid", U"Sound: To TextGrid...") {
	SENTENCE (U"All tier names", U"Mary John bell")
	SENTENCE (U"Which of these are point tiers?", U"bell")
	OK2
DO
	LOOP {
		iam (Sound);
		autoTextGrid thee = TextGrid_create (my xmin, my xmax, GET_STRING (U"All tier names"), GET_STRING (U"Which of these are point tiers?"));
		praat_new (thee.move(), my name);
	}
END2 }

DIRECT3 (NEW_Sound_to_TextTier) {
	LOOP {
		iam (Sound);
		autoTextTier thee = TextTier_create (my xmin, my xmax);
		praat_new (thee.move(), my name);
	}
END2 }

FORM3 (PREFS_SoundInputPrefs, U"Sound recording preferences", U"SoundRecorder") {
	NATURAL (U"Buffer size (MB)", U"60")
	OPTIONMENU_ENUM (U"Input sound system", kMelder_inputSoundSystem, DEFAULT)
	OK2
SET_INTEGER (U"Buffer size", SoundRecorder_getBufferSizePref_MB ())
SET_ENUM (U"Input sound system", kMelder_inputSoundSystem, MelderAudio_getInputSoundSystem())
DO
	long size = GET_INTEGER (U"Buffer size");
	if (size > 1000) Melder_throw (U"Buffer size cannot exceed 1000 megabytes.");
	SoundRecorder_setBufferSizePref_MB (size);
	MelderAudio_setInputSoundSystem (GET_ENUM (kMelder_inputSoundSystem, U"Input sound system"));
END2 }

FORM3 (PREFS_SoundOutputPrefs, U"Sound playing preferences", nullptr) {
	LABEL (U"", U"The following determines how sounds are played.")
	LABEL (U"", U"Between parentheses, you find what you can do simultaneously.")
	LABEL (U"", U"Decrease asynchronicity if sound plays with discontinuities.")
	OPTIONMENU_ENUM (U"Maximum asynchronicity", kMelder_asynchronicityLevel, DEFAULT)
	#define xstr(s) str(s)
	#define str(s) #s
	REAL (U"Silence before (s)", U"" xstr (kMelderAudio_outputSilenceBefore_DEFAULT))
	REAL (U"Silence after (s)", U"" xstr (kMelderAudio_outputSilenceAfter_DEFAULT))
	OPTIONMENU_ENUM (U"Output sound system", kMelder_outputSoundSystem, DEFAULT)
	OK2
SET_ENUM (U"Maximum asynchronicity", kMelder_asynchronicityLevel, MelderAudio_getOutputMaximumAsynchronicity ())
SET_REAL (U"Silence before", MelderAudio_getOutputSilenceBefore ())
SET_REAL (U"Silence after", MelderAudio_getOutputSilenceAfter ())
SET_ENUM (U"Output sound system", kMelder_outputSoundSystem, MelderAudio_getOutputSoundSystem())
DO
	MelderAudio_stopPlaying (MelderAudio_IMPLICIT);
	MelderAudio_setOutputMaximumAsynchronicity (GET_ENUM (kMelder_asynchronicityLevel, U"Maximum asynchronicity"));
	MelderAudio_setOutputSilenceBefore (GET_REAL (U"Silence before"));
	MelderAudio_setOutputSilenceAfter (GET_REAL (U"Silence after"));
	MelderAudio_setOutputSoundSystem (GET_ENUM (kMelder_outputSoundSystem, U"Output sound system"));
END2 }

#ifdef HAVE_PULSEAUDIO
void pulseAudio_serverReport ();
DIRECT3 (INFO_Praat_reportSoundServerProperties) {
	pulseAudio_serverReport ();
END2 }
#endif

FORM_WRITE3 (SAVE_Sound_saveAsAifcFile, U"Save as AIFC file", nullptr, U"aifc") {
	autoSoundAndLongSoundList list = SoundAndLongSoundList_create ();
	LOOP {
		iam (Sampled);
		list -> addItem_ref (me);
	}
	LongSound_concatenate (list.get(), file, Melder_AIFC, 16);
END2 }

FORM_WRITE3 (SAVE_Sound_saveAsAiffFile, U"Save as AIFF file", nullptr, U"aiff") {
	autoSoundAndLongSoundList list = SoundAndLongSoundList_create ();
	LOOP {
		iam (Sampled);
		list -> addItem_ref (me);
	}
	LongSound_concatenate (list.get(), file, Melder_AIFF, 16);
END2 }

FORM_WRITE3 (SAVE_Sound_saveAsFlacFile, U"Save as FLAC file", nullptr, U"flac") {
	autoSoundAndLongSoundList list = SoundAndLongSoundList_create ();
	LOOP {
		iam (Sampled);
		list -> addItem_ref (me);
	}
	LongSound_concatenate (list.get(), file, Melder_FLAC, 16);
END2 }

FORM_WRITE3 (SAVE_Sound_saveAsKayFile, U"Save as Kay sound file", nullptr, U"kay") {
	LOOP {
		iam (Sound);
		Sound_saveAsKayFile (me, file);
	}
END2 }

FORM_WRITE3 (SAVE_Sound_saveAsNextSunFile, U"Save as NeXT/Sun file", nullptr, U"au") {
	autoSoundAndLongSoundList list = SoundAndLongSoundList_create ();
	LOOP {
		iam (Sampled);
		list -> addItem_ref (me);
	}
	LongSound_concatenate (list.get(), file, Melder_NEXT_SUN, 16);
END2 }

FORM_WRITE3 (SAVE_Sound_saveAsNistFile, U"Save as NIST file", nullptr, U"nist") {
	autoSoundAndLongSoundList list = SoundAndLongSoundList_create ();
	LOOP {
		iam (Sampled);
		list -> addItem_ref (me);
	}
	LongSound_concatenate (list.get(), file, Melder_NIST, 16);
END2 }

FORM_WRITE3 (SAVE_Sound_saveAsRaw8bitSignedFile, U"Save as raw 8-bit signed sound file", nullptr, U"8sig") {
	LOOP {
		iam (Sound);
		Sound_saveAsRawSoundFile (me, file, Melder_LINEAR_8_SIGNED);
	}
END2 }

FORM_WRITE3 (SAVE_Sound_saveAsRaw8bitUnsignedFile, U"Save as raw 8-bit unsigned sound file", nullptr, U"8uns") {
	LOOP {
		iam (Sound);
		Sound_saveAsRawSoundFile (me, file, Melder_LINEAR_8_UNSIGNED);
	}
END2 }

FORM_WRITE3 (SAVE_Sound_saveAsRaw16bitBigEndianFile, U"Save as raw 16-bit big-endian sound file", nullptr, U"16be") {
	LOOP {
		iam (Sound);
		Sound_saveAsRawSoundFile (me, file, Melder_LINEAR_16_BIG_ENDIAN);
	}
END2 }

FORM_WRITE3 (SAVE_Sound_saveAsRaw16bitLittleEndianFile, U"Save as raw 16-bit little-endian sound file", nullptr, U"16le") {
	LOOP {
		iam (Sound);
		Sound_saveAsRawSoundFile (me, file, Melder_LINEAR_16_LITTLE_ENDIAN);
	}
END2 }

FORM_WRITE3 (SAVE_Sound_saveAsRaw24bitBigEndianFile, U"Save as raw 24-bit big-endian sound file", nullptr, U"24be") {
	LOOP {
		iam (Sound);
		Sound_saveAsRawSoundFile (me, file, Melder_LINEAR_24_BIG_ENDIAN);
	}
END2 }

FORM_WRITE3 (SAVE_Sound_saveAsRaw24bitLittleEndianFile, U"Save as raw 24-bit little-endian sound file", nullptr, U"24le") {
	LOOP {
		iam (Sound);
		Sound_saveAsRawSoundFile (me, file, Melder_LINEAR_24_LITTLE_ENDIAN);
	}
END2 }

FORM_WRITE3 (SAVE_Sound_saveAsRaw32bitBigEndianFile, U"Save as raw 32-bit big-endian sound file", nullptr, U"32be") {
	LOOP {
		iam (Sound);
		Sound_saveAsRawSoundFile (me, file, Melder_LINEAR_32_BIG_ENDIAN);
	}
END2 }

FORM_WRITE3 (SAVE_Sound_saveAsRaw32bitLittleEndianFile, U"Save as raw 32-bit little-endian sound file", nullptr, U"32le") {
	LOOP {
		iam (Sound);
		Sound_saveAsRawSoundFile (me, file, Melder_LINEAR_32_LITTLE_ENDIAN);
	}
END2 }

FORM_WRITE3 (SAVE_Sound_saveAsSesamFile, U"Save as Sesam file", nullptr, U"sdf") {
	LOOP {
		iam (Sound);
		Sound_saveAsSesamFile (me, file);
	}
END2 }

FORM_WRITE3 (SAVE_Sound_saveAsStereoAifcFile, U"Save as stereo AIFC file", nullptr, U"aifc") {
	OrderedOf<structSound> list;
	LOOP {
		iam (Sound);
		list. addItem_ref (me);
	}
	autoSound stereo = Sounds_combineToStereo (& list);
	Sound_saveAsAudioFile (stereo.get(), file, Melder_AIFC, 16);
END2 }

FORM_WRITE3 (SAVE_Sound_saveAsStereoAiffFile, U"Save as stereo AIFF file", nullptr, U"aiff") {
	OrderedOf<structSound> list;
	LOOP {
		iam (Sound);
		list. addItem_ref (me);
	}
	autoSound stereo = Sounds_combineToStereo (& list);
	Sound_saveAsAudioFile (stereo.get(), file, Melder_AIFF, 16);
END2 }

FORM_WRITE3 (SAVE_Sound_saveAsStereoNextSunFile, U"Save as stereo NeXT/Sun file", nullptr, U"au") {
	OrderedOf<structSound> list;
	LOOP {
		iam (Sound);
		list. addItem_ref (me);
	}
	autoSound stereo = Sounds_combineToStereo (& list);
	Sound_saveAsAudioFile (stereo.get(), file, Melder_NEXT_SUN, 16);
END2 }

FORM_WRITE3 (SAVE_Sound_saveAsStereoNistFile, U"Save as stereo NIST file", nullptr, U"nist") {
	OrderedOf<structSound> list;
	LOOP {
		iam (Sound);
		list. addItem_ref (me);
	}
	autoSound stereo = Sounds_combineToStereo (& list);
	Sound_saveAsAudioFile (stereo.get(), file, Melder_NIST, 16);
END2 }

FORM_WRITE3 (SAVE_Sound_saveAsStereoFlacFile, U"Save as stereo FLAC file", nullptr, U"flac") {
	OrderedOf<structSound> list;
	LOOP {
		iam (Sound);
		list. addItem_ref (me);
	}
	autoSound stereo = Sounds_combineToStereo (& list);
	Sound_saveAsAudioFile (stereo.get(), file, Melder_FLAC, 16);
END2 }

FORM_WRITE3 (SAVE_Sound_saveAsStereoWavFile, U"Save as stereo WAV file", nullptr, U"wav") {
	OrderedOf<structSound> list;
	LOOP {
		iam (Sound);
		list. addItem_ref (me);
	}
	autoSound stereo = Sounds_combineToStereo (& list);
	Sound_saveAsAudioFile (stereo.get(), file, Melder_WAV, 16);
END2 }

FORM_WRITE3 (SAVE_Sound_saveAsSunAudioFile, U"Save as NeXT/Sun file", nullptr, U"au") {
	autoSoundAndLongSoundList list = SoundAndLongSoundList_create ();
	LOOP {
		iam (Sampled);
		list -> addItem_ref (me);
	}
	LongSound_concatenate (list.get(), file, Melder_NEXT_SUN, 16);
END2 }

FORM_WRITE3 (SAVE_Sound_saveAsWavFile, U"Save as WAV file", nullptr, U"wav") {
	autoSoundAndLongSoundList list = SoundAndLongSoundList_create ();
	LOOP {
		iam (Sampled);
		list -> addItem_ref (me);
	}
	LongSound_concatenate (list.get(), file, Melder_WAV, 16);
END2 }

/***** STOP *****/

DIRECT3 (PLAY_stopPlayingSound) {
	MelderAudio_stopPlaying (MelderAudio_IMPLICIT);
END2 }

/***** Help menus *****/

DIRECT3 (HELP_AnnotationTutorial) {
	Melder_help (U"Intro 7. Annotation");
END2 }

DIRECT3 (HELP_FilteringTutorial) {
	Melder_help (U"Filtering");
END2 }

/***** file recognizers *****/

static autoDaata macSoundOrEmptyFileRecognizer (int nread, const char * /* header */, MelderFile file) {
	/***** No data in file? This may be a Macintosh sound file with only a resource fork. *****/
	if (nread > 0) return autoDaata ();
	Melder_throw (U"File ", file, U" contains no audio data.");   // !!!
}

static autoDaata soundFileRecognizer (int nread, const char *header, MelderFile file) {
	if (nread < 16) return autoDaata ();
	if (strnequ (header, "FORM", 4) && strnequ (header + 8, "AIF", 3)) return Sound_readFromSoundFile (file);
	if (strnequ (header, "RIFF", 4) && (strnequ (header + 8, "WAVE", 4) || strnequ (header + 8, "CDDA", 4))) return Sound_readFromSoundFile (file);
	if (strnequ (header, ".snd", 4)) return Sound_readFromSoundFile (file);
	if (strnequ (header, "NIST_1A", 7)) return Sound_readFromSoundFile (file);
	if (strnequ (header, "fLaC", 4)) return Sound_readFromSoundFile (file);   // Erez Volk, March 2007
	if ((Melder_stringMatchesCriterion (MelderFile_name (file), kMelder_string_ENDS_WITH, U".mp3") ||
	     Melder_stringMatchesCriterion (MelderFile_name (file), kMelder_string_ENDS_WITH, U".MP3"))
		&& mp3_recognize (nread, header)) return Sound_readFromSoundFile (file);   // Erez Volk, May 2007
	return autoDaata ();
}

static autoDaata movieFileRecognizer (int nread, const char * /* header */, MelderFile file) {
	const char32 *fileName = MelderFile_name (file);
	/*Melder_casual ("%d %d %d %d %d %d %d %d %d %d", header [0],
		header [1], header [2], header [3],
		header [4], header [5], header [6],
		header [7], header [8], header [9]);*/
	if (nread < 512 || (! Melder_stringMatchesCriterion (fileName, kMelder_string_ENDS_WITH, U".mov") &&
	                    ! Melder_stringMatchesCriterion (fileName, kMelder_string_ENDS_WITH, U".MOV") &&
	                    ! Melder_stringMatchesCriterion (fileName, kMelder_string_ENDS_WITH, U".avi") &&
	                    ! Melder_stringMatchesCriterion (fileName, kMelder_string_ENDS_WITH, U".AVI"))) return autoDaata ();
	Melder_throw (U"This Praat version cannot open movie files.");
	return autoDaata ();
}

static autoDaata sesamFileRecognizer (int nread, const char * /* header */, MelderFile file) {
	const char32 *fileName = MelderFile_name (file);
	if (nread < 512 || (! Melder_stringMatchesCriterion (fileName, kMelder_string_ENDS_WITH, U".sdf") &&
	                    ! Melder_stringMatchesCriterion (fileName, kMelder_string_ENDS_WITH, U".SDF"))) return autoDaata ();
	return Sound_readFromSesamFile (file);
}

static autoDaata bellLabsFileRecognizer (int nread, const char *header, MelderFile file) {
	if (nread < 16 || ! strnequ (& header [0], "SIG\n", 4)) return autoDaata ();
	return Sound_readFromBellLabsFile (file);
}

static autoDaata kayFileRecognizer (int nread, const char *header, MelderFile file) {
	if (nread <= 12 || ! strnequ (& header [0], "FORMDS16", 8)) return autoDaata ();
	return Sound_readFromKayFile (file);
}

/***** override play and record buttons in manuals *****/

static autoSound melderSound, melderSoundFromFile;
static Sound last;
static int recordProc (double duration) {
	if (last == melderSound.get()) last = nullptr;
	MelderAudio_stopPlaying (MelderAudio_IMPLICIT);
	melderSound = Sound_record_fixedTime (1, 1.0, 0.5, 44100, duration);
	if (! melderSound) return 0;
	last = melderSound.get();
	return 1;
}
static int recordFromFileProc (MelderFile file) {
	if (last == melderSoundFromFile.get()) last = nullptr;
	Melder_warningOff ();   // like "misssing samples"
	melderSoundFromFile = Data_readFromFile (file). static_cast_move<structSound>();
	Melder_warningOn ();
	if (! melderSoundFromFile) return 0;
	if (! Thing_isa (melderSoundFromFile.get(), classSound)) { melderSoundFromFile.reset(); return 0; }
	last = melderSoundFromFile.get();
	Sound_play (melderSoundFromFile.get(), nullptr, nullptr);
	return 1;
}
static void playProc () {
	if (melderSound) {
		Sound_play (melderSound.get(), nullptr, nullptr);
		last = melderSound.get();
	}
}
static void playReverseProc () {
	/*if (melderSound) Sound_playReverse (melderSound);*/
}
static int publishPlayedProc () {
	if (! last) return 0;
	autoSound sound = Data_copy (last);
	return Data_publish (sound.move());
}

/***** buttons *****/

void praat_uvafon_Sound_init ();
void praat_uvafon_Sound_init () {

	Data_recognizeFileType (macSoundOrEmptyFileRecognizer);
	Data_recognizeFileType (soundFileRecognizer);
	Data_recognizeFileType (movieFileRecognizer);
	Data_recognizeFileType (sesamFileRecognizer);
	Data_recognizeFileType (bellLabsFileRecognizer);
	Data_recognizeFileType (kayFileRecognizer);

	SoundRecorder_preferences ();
	structSoundRecorder           :: f_preferences ();
	structFunctionEditor          :: f_preferences ();
	LongSound_preferences ();
	structTimeSoundEditor         :: f_preferences ();
	structTimeSoundAnalysisEditor :: f_preferences ();

	Melder_setRecordProc (recordProc);
	Melder_setRecordFromFileProc (recordFromFileProc);
	Melder_setPlayProc (playProc);
	Melder_setPlayReverseProc (playReverseProc);
	Melder_setPublishPlayedProc (publishPlayedProc);

	praat_addMenuCommand (U"Objects", U"New", U"Record mono Sound...", nullptr, praat_ATTRACTIVE | 'R' | praat_NO_API, WINDOW_Sound_recordMono);
	praat_addMenuCommand (U"Objects", U"New", U"Record stereo Sound...", nullptr, praat_NO_API, WINDOW_Sound_recordStereo);
	praat_addMenuCommand (U"Objects", U"New", U"Record Sound (fixed time)...", nullptr, praat_HIDDEN | praat_FORCE_API, RECORD1_Sound_record_fixedTime);
	praat_addMenuCommand (U"Objects", U"New", U"Sound", nullptr, 0, nullptr);
		praat_addMenuCommand (U"Objects", U"New", U"Create Sound as pure tone...", nullptr, 1, NEW1_Sound_createAsPureTone);
		praat_addMenuCommand (U"Objects", U"New", U"Create Sound from formula...", nullptr, 1, NEW1_Sound_createFromFormula);
		praat_addMenuCommand (U"Objects", U"New",   U"Create Sound...", U"*Create Sound from formula...", praat_DEPTH_1 | praat_DEPRECATED_2007, NEW1_Sound_create);
		praat_addMenuCommand (U"Objects", U"New", U"-- create sound advanced --", nullptr, 1, nullptr);
		praat_addMenuCommand (U"Objects", U"New", U"Create Sound as tone complex...", nullptr, 1, NEW1_Sound_createFromToneComplex);
		praat_addMenuCommand (U"Objects", U"New",   U"Create Sound from tone complex...", U"*Create Sound as tone complex...", praat_DEPTH_1 | praat_DEPRECATED_2013, NEW1_Sound_createFromToneComplex);

	praat_addMenuCommand (U"Objects", U"Open", U"-- read sound --", nullptr, 0, nullptr);
	praat_addMenuCommand (U"Objects", U"Open", U"Open long sound file...", nullptr, 'L', READ1_LongSound_open);
	praat_addMenuCommand (U"Objects", U"Open", U"Read two Sounds from stereo file...", nullptr, praat_DEPRECATED_2010, READMANY_Sound_readSeparateChannelsFromSoundFile);
	praat_addMenuCommand (U"Objects", U"Open", U"Read separate channels from sound file...", nullptr, 0, READMANY_Sound_readSeparateChannelsFromSoundFile);
	praat_addMenuCommand (U"Objects", U"Open", U"Read from special sound file", nullptr, 0, nullptr);
		praat_addMenuCommand (U"Objects", U"Open", U"Read Sound from raw Alaw file...", nullptr, praat_DEPTH_1, READ1_Sound_readFromRawAlawFile);

	praat_addMenuCommand (U"Objects", U"Goodies", U"Stop playing sound", nullptr, GuiMenu_ESCAPE, PLAY_stopPlayingSound);
	praat_addMenuCommand (U"Objects", U"Preferences", U"-- sound prefs --", nullptr, 0, nullptr);
	praat_addMenuCommand (U"Objects", U"Preferences", U"Sound recording preferences...", nullptr, 0, PREFS_SoundInputPrefs);
	praat_addMenuCommand (U"Objects", U"Preferences", U"Sound playing preferences...", nullptr, 0, PREFS_SoundOutputPrefs);
	praat_addMenuCommand (U"Objects", U"Preferences", U"LongSound preferences...", nullptr, 0, PREFS_LongSoundPrefs);
#ifdef HAVE_PULSEAUDIO
	praat_addMenuCommand (U"Objects", U"Technical", U"Report sound server properties", U"Report system properties", 0, INFO_Praat_reportSoundServerProperties);
#endif

	praat_addAction1 (classLongSound, 0, U"LongSound help", nullptr, 0, HELP_LongSound_help);
	praat_addAction1 (classLongSound, 1, U"View", nullptr, praat_ATTRACTIVE, WINDOW_LongSound_view);
	praat_addAction1 (classLongSound, 1,   U"Open", U"*View", praat_DEPRECATED_2011, WINDOW_LongSound_view);
	praat_addAction1 (classLongSound, 0, U"Play part...", nullptr, 0, PLAY_LongSound_playPart);
	praat_addAction1 (classLongSound, 1, U"Query -", nullptr, 0, nullptr);
		praat_TimeFunction_query_init (classLongSound);
		praat_addAction1 (classLongSound, 1, U"Sampling", nullptr, 1, nullptr);
		praat_addAction1 (classLongSound, 1, U"Get number of samples", nullptr, 2, INTEGER_LongSound_getNumberOfSamples);
		praat_addAction1 (classLongSound, 1, U"Get sampling period", nullptr, 2, REAL_LongSound_getSamplePeriod);
		praat_addAction1 (classLongSound, 1,   U"Get sample duration", U"*Get sampling period", praat_DEPTH_2 | praat_DEPRECATED_2004, REAL_LongSound_getSamplePeriod);
		praat_addAction1 (classLongSound, 1,   U"Get sample period", U"*Get sampling period", praat_DEPTH_2 | praat_DEPRECATED_2004, REAL_LongSound_getSamplePeriod);
		praat_addAction1 (classLongSound, 1, U"Get sampling frequency", nullptr, 2, REAL_LongSound_getSampleRate);
		praat_addAction1 (classLongSound, 1,   U"Get sample rate", U"*Get sampling frequency", praat_DEPTH_2 | praat_DEPRECATED_2004, REAL_LongSound_getSampleRate);
		praat_addAction1 (classLongSound, 1, U"-- get time discretization --", nullptr, 2, nullptr);
		praat_addAction1 (classLongSound, 1, U"Get time from sample number...", nullptr, 2, REAL_LongSound_getTimeFromIndex);
		praat_addAction1 (classLongSound, 1,   U"Get time from index...", U"*Get time from sample number...", praat_DEPTH_2 | praat_DEPRECATED_2004, REAL_LongSound_getTimeFromIndex);
		praat_addAction1 (classLongSound, 1, U"Get sample number from time...", nullptr, 2, REAL_LongSound_getIndexFromTime);
		praat_addAction1 (classLongSound, 1,   U"Get index from time...", U"*Get sample number from time...", praat_DEPTH_2 | praat_DEPRECATED_2004, REAL_LongSound_getIndexFromTime);
	praat_addAction1 (classLongSound, 0, U"Annotate -", nullptr, 0, nullptr);
		praat_addAction1 (classLongSound, 0, U"Annotation tutorial", nullptr, 1, HELP_AnnotationTutorial);
		praat_addAction1 (classLongSound, 0, U"-- to text grid --", nullptr, 1, nullptr);
		praat_addAction1 (classLongSound, 0, U"To TextGrid...", nullptr, 1, NEW_LongSound_to_TextGrid);
	praat_addAction1 (classLongSound, 0, U"Convert to Sound", nullptr, 0, nullptr);
	praat_addAction1 (classLongSound, 0, U"Extract part...", nullptr, 0, NEW_LongSound_extractPart);
	praat_addAction1 (classLongSound, 0, U"Concatenate?", nullptr, 0, INFO_LongSound_concatenate);
	praat_addAction1 (classLongSound, 0, U"Save as WAV file...", nullptr, 0, SAVE_LongSound_saveAsWavFile);
	praat_addAction1 (classLongSound, 0,   U"Write to WAV file...", U"*Save as WAV file...", praat_DEPRECATED_2011, SAVE_LongSound_saveAsWavFile);
	praat_addAction1 (classLongSound, 0, U"Save as AIFF file...", nullptr, 0, SAVE_LongSound_saveAsAiffFile);
	praat_addAction1 (classLongSound, 0,   U"Write to AIFF file...", U"*Save as AIFF file...", praat_DEPRECATED_2011, SAVE_LongSound_saveAsAiffFile);
	praat_addAction1 (classLongSound, 0, U"Save as AIFC file...", nullptr, 0, SAVE_LongSound_saveAsAifcFile);
	praat_addAction1 (classLongSound, 0,   U"Write to AIFC file...", U"*Save as AIFC file...", praat_DEPRECATED_2011, SAVE_LongSound_saveAsAifcFile);
	praat_addAction1 (classLongSound, 0, U"Save as Next/Sun file...", nullptr, 0, SAVE_LongSound_saveAsNextSunFile);
	praat_addAction1 (classLongSound, 0,   U"Write to Next/Sun file...", U"*Save as Next/Sun file...", praat_DEPRECATED_2011, SAVE_LongSound_saveAsNextSunFile);
	praat_addAction1 (classLongSound, 0, U"Save as NIST file...", nullptr, 0, SAVE_LongSound_saveAsNistFile);
	praat_addAction1 (classLongSound, 0,   U"Write to NIST file...", U"*Save as NIST file...", praat_DEPRECATED_2011, SAVE_LongSound_saveAsNistFile);
	praat_addAction1 (classLongSound, 0, U"Save as FLAC file...", nullptr, 0, SAVE_LongSound_saveAsFlacFile);
	praat_addAction1 (classLongSound, 0,   U"Write to FLAC file...", U"*Save as FLAC file...", praat_DEPRECATED_2011, SAVE_LongSound_saveAsFlacFile);
	praat_addAction1 (classLongSound, 0, U"Save left channel as WAV file...", nullptr, 0, SAVE_LongSound_saveLeftChannelAsWavFile);
	praat_addAction1 (classLongSound, 0,   U"Write left channel to WAV file...", U"*Save left channel as WAV file...", praat_DEPRECATED_2011, SAVE_LongSound_saveLeftChannelAsWavFile);
	praat_addAction1 (classLongSound, 0, U"Save left channel as AIFF file...", nullptr, 0, SAVE_LongSound_saveLeftChannelAsAiffFile);
	praat_addAction1 (classLongSound, 0,   U"Write left channel to AIFF file...", U"*Save left channel as AIFF file...", praat_DEPRECATED_2011, SAVE_LongSound_saveLeftChannelAsAiffFile);
	praat_addAction1 (classLongSound, 0, U"Save left channel as AIFC file...", nullptr, 0, SAVE_LongSound_saveLeftChannelAsAifcFile);
	praat_addAction1 (classLongSound, 0,   U"Write left channel to AIFC file...", U"*Save left channel as AIFC file...", praat_DEPRECATED_2011, SAVE_LongSound_saveLeftChannelAsAifcFile);
	praat_addAction1 (classLongSound, 0, U"Save left channel as Next/Sun file...", nullptr, 0, SAVE_LongSound_saveLeftChannelAsNextSunFile);
	praat_addAction1 (classLongSound, 0,   U"Write left channel to Next/Sun file...", U"*Save left channel as Next/Sun file...", praat_DEPRECATED_2011, SAVE_LongSound_saveLeftChannelAsNextSunFile);
	praat_addAction1 (classLongSound, 0, U"Save left channel as NIST file...", nullptr, 0, SAVE_LongSound_saveLeftChannelAsNistFile);
	praat_addAction1 (classLongSound, 0,   U"Write left channel to NIST file...", U"*Save left channel as NIST file...", praat_DEPRECATED_2011, SAVE_LongSound_saveLeftChannelAsNistFile);
	praat_addAction1 (classLongSound, 0, U"Save left channel as FLAC file...", nullptr, 0, SAVE_LongSound_saveLeftChannelAsFlacFile);
	praat_addAction1 (classLongSound, 0,   U"Write left channel to FLAC file...", U"*Save left channel as FLAC file...", praat_DEPRECATED_2011, SAVE_LongSound_saveLeftChannelAsFlacFile);
	praat_addAction1 (classLongSound, 0, U"Save right channel as WAV file...", nullptr, 0, SAVE_LongSound_saveRightChannelAsWavFile);
	praat_addAction1 (classLongSound, 0,   U"Write right channel to WAV file...", U"*Save right channel as WAV file...", praat_DEPRECATED_2011, SAVE_LongSound_saveRightChannelAsWavFile);
	praat_addAction1 (classLongSound, 0, U"Save right channel as AIFF file...", nullptr, 0, SAVE_LongSound_saveRightChannelAsAiffFile);
	praat_addAction1 (classLongSound, 0,   U"Write right channel to AIFF file...", U"*Save right channel as AIFF file...", praat_DEPRECATED_2011, SAVE_LongSound_saveRightChannelAsAiffFile);
	praat_addAction1 (classLongSound, 0, U"Save right channel as AIFC file...", nullptr, 0, SAVE_LongSound_saveRightChannelAsAifcFile);
	praat_addAction1 (classLongSound, 0,   U"Write right channel to AIFC file...", U"*Save right channel as AIFC file...", praat_DEPRECATED_2011, SAVE_LongSound_saveRightChannelAsAifcFile);
	praat_addAction1 (classLongSound, 0, U"Save right channel as Next/Sun file...", nullptr, 0, SAVE_LongSound_saveRightChannelAsNextSunFile);
	praat_addAction1 (classLongSound, 0,   U"Write right channel to Next/Sun file...", U"*Save right channel as Next/Sun file...", praat_DEPRECATED_2011, SAVE_LongSound_saveRightChannelAsNextSunFile);
	praat_addAction1 (classLongSound, 0, U"Save right channel as NIST file...", nullptr, 0, SAVE_LongSound_saveRightChannelAsNistFile);
	praat_addAction1 (classLongSound, 0,   U"Write right channel to NIST file...", U"*Save right channel as NIST file...", praat_DEPRECATED_2011, SAVE_LongSound_saveRightChannelAsNistFile);
	praat_addAction1 (classLongSound, 0, U"Save right channel as FLAC file...", nullptr, 0, SAVE_LongSound_saveRightChannelAsFlacFile);
	praat_addAction1 (classLongSound, 0,   U"Write right channel to FLAC file...", U"*Save right channel as FLAC file...", praat_DEPRECATED_2011, SAVE_LongSound_saveRightChannelAsFlacFile);
	praat_addAction1 (classLongSound, 0, U"Save part as audio file...", nullptr, 0, SAVE_LongSound_savePartAsAudioFile);
	praat_addAction1 (classLongSound, 0,   U"Write part to audio file...", U"*Save part as audio file...", praat_DEPRECATED_2011, SAVE_LongSound_savePartAsAudioFile);

	praat_addAction1 (classSound, 0, U"Save as WAV file...", nullptr, 0, SAVE_Sound_saveAsWavFile);
	praat_addAction1 (classSound, 0,   U"Write to WAV file...", U"*Save as WAV file...", praat_DEPRECATED_2011, SAVE_Sound_saveAsWavFile);
	praat_addAction1 (classSound, 0, U"Save as AIFF file...", nullptr, 0, SAVE_Sound_saveAsAiffFile);
	praat_addAction1 (classSound, 0,   U"Write to AIFF file...", U"*Save as AIFF file...", praat_DEPRECATED_2011, SAVE_Sound_saveAsAiffFile);
	praat_addAction1 (classSound, 0, U"Save as AIFC file...", nullptr, 0, SAVE_Sound_saveAsAifcFile);
	praat_addAction1 (classSound, 0,   U"Write to AIFC file...", U"*Save as AIFC file...", praat_DEPRECATED_2011, SAVE_Sound_saveAsAifcFile);
	praat_addAction1 (classSound, 0, U"Save as Next/Sun file...", nullptr, 0, SAVE_Sound_saveAsNextSunFile);
	praat_addAction1 (classSound, 0,   U"Write to Next/Sun file...", U"*Save as Next/Sun file...", praat_DEPRECATED_2011, SAVE_Sound_saveAsNextSunFile);
	praat_addAction1 (classSound, 0, U"Save as Sun audio file...", nullptr, praat_HIDDEN, SAVE_Sound_saveAsSunAudioFile);
	praat_addAction1 (classSound, 0,   U"Write to Sun audio file...", U"*Save as Sun audio file...", praat_DEPRECATED_2011, SAVE_Sound_saveAsSunAudioFile);
	praat_addAction1 (classSound, 0, U"Save as NIST file...", nullptr, 0, SAVE_Sound_saveAsNistFile);
	praat_addAction1 (classSound, 0,   U"Write to NIST file...", U"*Save as NIST file...", praat_DEPRECATED_2011, SAVE_Sound_saveAsNistFile);
	praat_addAction1 (classSound, 0, U"Save as FLAC file...", nullptr, 0, SAVE_Sound_saveAsFlacFile);
	praat_addAction1 (classSound, 0,   U"Write to FLAC file...", U"*Save as FLAC file...", praat_DEPRECATED_2011, SAVE_Sound_saveAsFlacFile);
	praat_addAction1 (classSound, 1, U"Save as Kay sound file...", nullptr, 0, SAVE_Sound_saveAsKayFile);
	praat_addAction1 (classSound, 1,   U"Write to Kay sound file...", U"*Save as Kay sound file...", praat_DEPRECATED_2011, SAVE_Sound_saveAsKayFile);
	praat_addAction1 (classSound, 1, U"Save as Sesam file...", nullptr, praat_HIDDEN, SAVE_Sound_saveAsSesamFile);
	praat_addAction1 (classSound, 1,   U"Write to Sesam file...", U"*Save as Sesam file...", praat_DEPRECATED_2011, SAVE_Sound_saveAsSesamFile);
	praat_addAction1 (classSound, 0, U"Save as 24-bit WAV file...", nullptr, 0, SAVE_Sound_saveAs24BitWavFile);
	praat_addAction1 (classSound, 0, U"Save as 32-bit WAV file...", nullptr, 0, SAVE_Sound_saveAs32BitWavFile);
	praat_addAction1 (classSound, 2,   U"Write to stereo WAV file...", U"* \"Combine to stereo\" and \"Save to WAV file...\"", praat_DEPRECATED_2007, SAVE_Sound_saveAsStereoWavFile);
	praat_addAction1 (classSound, 2,   U"Write to stereo AIFF file...", U"* \"Combine to stereo\" and \"Save to AIFF file...\"", praat_DEPRECATED_2007, SAVE_Sound_saveAsStereoAiffFile);
	praat_addAction1 (classSound, 2,   U"Write to stereo AIFC file...", U"* \"Combine to stereo\" and \"Save to AIFC file...\"", praat_DEPRECATED_2007, SAVE_Sound_saveAsStereoAifcFile);
	praat_addAction1 (classSound, 2,   U"Write to stereo Next/Sun file...", U"* \"Combine to stereo\" and \"Save to Next/Sun file...\"", praat_DEPRECATED_2007, SAVE_Sound_saveAsStereoNextSunFile);
	praat_addAction1 (classSound, 2,   U"Write to stereo NIST file...", U"* \"Combine to stereo\" and \"Save to NIST file...\"", praat_DEPRECATED_2007, SAVE_Sound_saveAsStereoNistFile);
	praat_addAction1 (classSound, 2,   U"Write to stereo FLAC file...", U"* \"Combine to stereo\" and \"Save to FLAC file...\"", praat_DEPRECATED_2007, SAVE_Sound_saveAsStereoFlacFile);
	//praat_addAction1 (classSound, 1, U"Save as raw sound file", nullptr, 0, nullptr);
	praat_addAction1 (classSound, 1, U"Save as raw 8-bit signed file...", nullptr, 0, SAVE_Sound_saveAsRaw8bitSignedFile);
	praat_addAction1 (classSound, 1,   U"Write to raw 8-bit signed file...", U"*Save as raw 8-bit signed file...", praat_DEPRECATED_2011, SAVE_Sound_saveAsRaw8bitSignedFile);
	praat_addAction1 (classSound, 1, U"Save as raw 8-bit unsigned file...", nullptr, 0, SAVE_Sound_saveAsRaw8bitUnsignedFile);
	praat_addAction1 (classSound, 1,   U"Write to raw 8-bit unsigned file...", U"*Save as raw 8-bit unsigned file...", praat_DEPRECATED_2011, SAVE_Sound_saveAsRaw8bitUnsignedFile);
	praat_addAction1 (classSound, 1, U"Save as raw 16-bit big-endian file...", nullptr, 0, SAVE_Sound_saveAsRaw16bitBigEndianFile);
	praat_addAction1 (classSound, 1, U"Save as raw 16-bit little-endian file...", nullptr, 0, SAVE_Sound_saveAsRaw16bitLittleEndianFile);
	praat_addAction1 (classSound, 1, U"Save as raw 24-bit big-endian file...", nullptr, 0, SAVE_Sound_saveAsRaw24bitBigEndianFile);
	praat_addAction1 (classSound, 1, U"Save as raw 24-bit little-endian file...", nullptr, 0, SAVE_Sound_saveAsRaw24bitLittleEndianFile);
	praat_addAction1 (classSound, 1, U"Save as raw 32-bit big-endian file...", nullptr, 0, SAVE_Sound_saveAsRaw32bitBigEndianFile);
	praat_addAction1 (classSound, 1, U"Save as raw 32-bit little-endian file...", nullptr, 0, SAVE_Sound_saveAsRaw32bitLittleEndianFile);
	praat_addAction1 (classSound, 0, U"Sound help", nullptr, 0, HELP_Sound_help);
	praat_addAction1 (classSound, 1, U"View & Edit", 0, praat_ATTRACTIVE | praat_NO_API, WINDOW_Sound_viewAndEdit);
	praat_addAction1 (classSound, 1,   U"Edit", U"*View & Edit", praat_DEPRECATED_2011 | praat_NO_API, WINDOW_Sound_viewAndEdit);
	praat_addAction1 (classSound, 1,   U"Open", U"*View & Edit", praat_DEPRECATED_2011 | praat_NO_API, WINDOW_Sound_viewAndEdit);
	praat_addAction1 (classSound, 0, U"Play", nullptr, 0, PLAY_Sound_play);
	praat_addAction1 (classSound, 1, U"Draw -", nullptr, 0, nullptr);
		praat_addAction1 (classSound, 0, U"Draw...", nullptr, 1, GRAPHICS_Sound_draw);
	praat_addAction1 (classSound, 1, U"Query -", nullptr, 0, nullptr);
		praat_TimeFunction_query_init (classSound);
		praat_addAction1 (classSound, 1, U"Get number of channels", nullptr, 1, INTEGER_Sound_getNumberOfChannels);
		praat_addAction1 (classSound, 1, U"Query time sampling", nullptr, 1, nullptr);
		praat_addAction1 (classSound, 1, U"Get number of samples", nullptr, 2, INTEGER_Sound_getNumberOfSamples);
		praat_addAction1 (classSound, 1, U"Get sampling period", nullptr, 2, REAL_Sound_getSamplePeriod);
		praat_addAction1 (classSound, 1,   U"Get sample duration", U"*Get sampling period", praat_DEPTH_2 | praat_DEPRECATED_2004, REAL_Sound_getSamplePeriod);
		praat_addAction1 (classSound, 1,   U"Get sample period", U"*Get sampling period", praat_DEPTH_2 | praat_DEPRECATED_2004, REAL_Sound_getSamplePeriod);
		praat_addAction1 (classSound, 1, U"Get sampling frequency", nullptr, 2, REAL_Sound_getSampleRate);
		praat_addAction1 (classSound, 1,   U"Get sample rate", U"*Get sampling frequency", praat_DEPTH_2 | praat_DEPRECATED_2004, REAL_Sound_getSampleRate);
		praat_addAction1 (classSound, 1, U"-- get time discretization --", nullptr, 2, nullptr);
		praat_addAction1 (classSound, 1, U"Get time from sample number...", nullptr, 2, REAL_Sound_getTimeFromIndex);
		praat_addAction1 (classSound, 1,   U"Get time from index...", U"*Get time from sample number...", praat_DEPTH_2 | praat_DEPRECATED_2004, REAL_Sound_getTimeFromIndex);
		praat_addAction1 (classSound, 1, U"Get sample number from time...", nullptr, 2, REAL_Sound_getIndexFromTime);
		praat_addAction1 (classSound, 1,   U"Get index from time...", U"*Get sample number from time...", praat_DEPTH_2 | praat_DEPRECATED_2004, REAL_Sound_getIndexFromTime);
		praat_addAction1 (classSound, 1, U"-- get content --", nullptr, 1, nullptr);
		praat_addAction1 (classSound, 1, U"Get value at time...", nullptr, 1, REAL_Sound_getValueAtTime);
		praat_addAction1 (classSound, 1, U"Get value at sample number...", nullptr, 1, REAL_Sound_getValueAtIndex);
		praat_addAction1 (classSound, 1,   U"Get value at index...", U"*Get value at sample number...", praat_DEPTH_1 | praat_DEPRECATED_2004, REAL_Sound_getValueAtIndex);
		praat_addAction1 (classSound, 1, U"-- get shape --", nullptr, 1, nullptr);
		praat_addAction1 (classSound, 1, U"Get minimum...", nullptr, 1, REAL_Sound_getMinimum);
		praat_addAction1 (classSound, 1, U"Get time of minimum...", nullptr, 1, REAL_Sound_getTimeOfMinimum);
		praat_addAction1 (classSound, 1, U"Get maximum...", nullptr, 1, REAL_Sound_getMaximum);
		praat_addAction1 (classSound, 1, U"Get time of maximum...", nullptr, 1, REAL_Sound_getTimeOfMaximum);
		praat_addAction1 (classSound, 1, U"Get absolute extremum...", nullptr, 1, REAL_Sound_getAbsoluteExtremum);
		praat_addAction1 (classSound, 1, U"Get nearest zero crossing...", nullptr, 1, REAL_Sound_getNearestZeroCrossing);
		praat_addAction1 (classSound, 1, U"-- get statistics --", nullptr, 1, nullptr);
		praat_addAction1 (classSound, 1, U"Get mean...", nullptr, 1, REAL_Sound_getMean);
		praat_addAction1 (classSound, 1, U"Get root-mean-square...", nullptr, 1, REAL_Sound_getRootMeanSquare);
		praat_addAction1 (classSound, 1, U"Get standard deviation...", nullptr, 1, REAL_Sound_getStandardDeviation);
		praat_addAction1 (classSound, 1, U"-- get energy --", nullptr, 1, nullptr);
		praat_addAction1 (classSound, 1, U"Get energy...", nullptr, 1, REAL_Sound_getEnergy);
		praat_addAction1 (classSound, 1, U"Get power...", nullptr, 1, REAL_Sound_getPower);
		praat_addAction1 (classSound, 1, U"-- get energy in air --", nullptr, 1, nullptr);
		praat_addAction1 (classSound, 1, U"Get energy in air", nullptr, 1, REAL_Sound_getEnergyInAir);
		praat_addAction1 (classSound, 1, U"Get power in air", nullptr, 1, REAL_Sound_getPowerInAir);
		praat_addAction1 (classSound, 1, U"Get intensity (dB)", nullptr, 1, REAL_Sound_getIntensity_dB);
	praat_addAction1 (classSound, 0, U"Modify -", nullptr, 0, nullptr);
		praat_TimeFunction_modify_init (classSound);
		praat_addAction1 (classSound, 0, U"-- modify generic --", nullptr, 1, nullptr);
		praat_addAction1 (classSound, 0, U"Reverse", nullptr, 1, MODIFY_Sound_reverse);
		praat_addAction1 (classSound, 0, U"Formula...", nullptr, 1, MODIFY_Sound_formula);
		praat_addAction1 (classSound, 0, U"Formula (part)...", nullptr, 1, MODIFY_Sound_formula_part);
		praat_addAction1 (classSound, 0, U"-- add & mul --", nullptr, 1, nullptr);
		praat_addAction1 (classSound, 0, U"Add...", nullptr, 1, MODIFY_Sound_add);
		praat_addAction1 (classSound, 0, U"Subtract mean", nullptr, 1, MODIFY_Sound_subtractMean);
		praat_addAction1 (classSound, 0, U"Multiply...", nullptr, 1, MODIFY_Sound_multiply);
		praat_addAction1 (classSound, 0, U"Multiply by window...", nullptr, 1, MODIFY_Sound_multiplyByWindow);
		praat_addAction1 (classSound, 0, U"Scale peak...", nullptr, 1, MODIFY_Sound_scalePeak);
		praat_addAction1 (classSound, 0,   U"Scale...", nullptr, praat_DEPTH_1 | praat_DEPRECATED_2004, MODIFY_Sound_scalePeak);
		praat_addAction1 (classSound, 0, U"Scale intensity...", nullptr, 1, MODIFY_Sound_scaleIntensity);
		praat_addAction1 (classSound, 0, U"-- set --", nullptr, 1, nullptr);
		praat_addAction1 (classSound, 0, U"Set value at sample number...", nullptr, 1, MODIFY_Sound_setValueAtIndex);
		praat_addAction1 (classSound, 0,   U"Set value at index...", U"*Set value at sample number...", praat_DEPTH_1 | praat_DEPRECATED_2004, MODIFY_Sound_setValueAtIndex);
		praat_addAction1 (classSound, 0, U"Set part to zero...", nullptr, 1, MODIFY_Sound_setPartToZero);
		praat_addAction1 (classSound, 0, U"-- modify hack --", nullptr, 1, nullptr);
		praat_addAction1 (classSound, 0, U"Override sampling frequency...", nullptr, 1, MODIFY_Sound_overrideSamplingFrequency);
		praat_addAction1 (classSound, 0,   U"Override sample rate...", U"*Override sampling frequency...", praat_DEPTH_1 | praat_DEPRECATED_2004, MODIFY_Sound_overrideSamplingFrequency);
		praat_addAction1 (classSound, 0, U"-- in-line filters --", nullptr, 1, nullptr);
		praat_addAction1 (classSound, 0, U"In-line filters", nullptr, 1, nullptr);
		praat_addAction1 (classSound, 0, U"Filter with one formant (in-line)...", nullptr, 2, MODIFY_Sound_filterWithOneFormantInline);
		praat_addAction1 (classSound, 0, U"Pre-emphasize (in-line)...", nullptr, 2, MODIFY_Sound_preemphasizeInline);
		praat_addAction1 (classSound, 0, U"De-emphasize (in-line)...", nullptr, 2, MODIFY_Sound_deemphasizeInline);
	praat_addAction1 (classSound, 0, U"Annotate -", nullptr, 0, nullptr);
		praat_addAction1 (classSound, 0, U"Annotation tutorial", nullptr, 1, HELP_AnnotationTutorial);
		praat_addAction1 (classSound, 0, U"-- to text grid --", nullptr, 1, nullptr);
		praat_addAction1 (classSound, 0, U"To TextGrid...", nullptr, 1, NEW_Sound_to_TextGrid);
		praat_addAction1 (classSound, 0, U"To TextTier", nullptr, praat_DEPTH_1 | praat_HIDDEN, NEW_Sound_to_TextTier);
		praat_addAction1 (classSound, 0, U"To IntervalTier", nullptr, praat_DEPTH_1 | praat_HIDDEN, NEW_Sound_to_IntervalTier);
	praat_addAction1 (classSound, 0, U"Analyse periodicity -", nullptr, 0, nullptr);
		praat_addAction1 (classSound, 0, U"To Pitch...", nullptr, 1, NEW_Sound_to_Pitch);
		praat_addAction1 (classSound, 0, U"To Pitch (ac)...", nullptr, 1, NEW_Sound_to_Pitch_ac);
		praat_addAction1 (classSound, 0, U"To Pitch (cc)...", nullptr, 1, NEW_Sound_to_Pitch_cc);
		praat_addAction1 (classSound, 0, U"To PointProcess (periodic, cc)...", nullptr, 1, NEW_Sound_to_PointProcess_periodic_cc);
		praat_addAction1 (classSound, 0, U"To PointProcess (periodic, peaks)...", nullptr, 1, NEW_Sound_to_PointProcess_periodic_peaks);
		praat_addAction1 (classSound, 0, U"-- points --", nullptr, 1, nullptr);
		praat_addAction1 (classSound, 0, U"To PointProcess (extrema)...", nullptr, 1, NEW_Sound_to_PointProcess_extrema);
		praat_addAction1 (classSound, 0, U"To PointProcess (zeroes)...", nullptr, 1, NEW_Sound_to_PointProcess_zeroes);
		praat_addAction1 (classSound, 0, U"-- hnr --", nullptr, 1, nullptr);
		praat_addAction1 (classSound, 0, U"To Harmonicity (cc)...", nullptr, 1, NEW_Sound_to_Harmonicity_cc);
		praat_addAction1 (classSound, 0, U"To Harmonicity (ac)...", nullptr, 1, NEW_Sound_to_Harmonicity_ac);
		praat_addAction1 (classSound, 0, U"To Harmonicity (gne)...", nullptr, 1, NEW_Sound_to_Harmonicity_gne);
		praat_addAction1 (classSound, 0, U"-- autocorrelation --", nullptr, 1, nullptr);
		praat_addAction1 (classSound, 0, U"Autocorrelate...", nullptr, 1, NEW_Sound_autoCorrelate);
	praat_addAction1 (classSound, 0, U"Analyse spectrum -", nullptr, 0, nullptr);
		praat_addAction1 (classSound, 0, U"To Spectrum...", nullptr, 1, NEW_Sound_to_Spectrum);
		praat_addAction1 (classSound, 0,   U"To Spectrum (fft)", U"*To Spectrum...", praat_DEPTH_1 | praat_DEPRECATED_2004, NEW_Sound_to_Spectrum_fft);
		praat_addAction1 (classSound, 0,   U"To Spectrum", U"*To Spectrum...", praat_DEPTH_1 | praat_DEPRECATED_2004, NEW_Sound_to_Spectrum_fft);
		praat_addAction1 (classSound, 0,   U"To Spectrum (dft)", U"*To Spectrum...", praat_DEPTH_1 | praat_DEPRECATED_2004, NEW_Sound_to_Spectrum_dft);
		praat_addAction1 (classSound, 0, U"To Ltas...", nullptr, 1, NEW_Sound_to_Ltas);
		praat_addAction1 (classSound, 0, U"To Ltas (pitch-corrected)...", nullptr, 1, NEW_Sound_to_Ltas_pitchCorrected);
		praat_addAction1 (classSound, 0, U"-- spectrotemporal --", nullptr, 1, nullptr);
		praat_addAction1 (classSound, 0, U"To Spectrogram...", nullptr, 1, NEW_Sound_to_Spectrogram);
		praat_addAction1 (classSound, 0, U"To Cochleagram...", nullptr, 1, NEW_Sound_to_Cochleagram);
		praat_addAction1 (classSound, 0, U"To Cochleagram (edb)...", nullptr, praat_DEPTH_1 | praat_HIDDEN, NEW_Sound_to_Cochleagram_edb);
		praat_addAction1 (classSound, 0, U"-- formants --", nullptr, 1, nullptr);
		praat_addAction1 (classSound, 0, U"To Formant (burg)...", nullptr, 1, NEW_Sound_to_Formant_burg);
		praat_addAction1 (classSound, 0, U"To Formant (hack)", nullptr, 1, nullptr);
		praat_addAction1 (classSound, 0, U"To Formant (keep all)...", nullptr, 2, NEW_Sound_to_Formant_keepAll);
		praat_addAction1 (classSound, 0, U"To Formant (sl)...", nullptr, 2, NEW_Sound_to_Formant_willems);
	praat_addAction1 (classSound, 0, U"To Intensity...", nullptr, 0, NEW_Sound_to_Intensity);
	praat_addAction1 (classSound, 0, U"To IntensityTier...", nullptr, praat_HIDDEN, NEW_Sound_to_IntensityTier);
	praat_addAction1 (classSound, 0, U"Manipulate -", nullptr, 0, nullptr);
	praat_addAction1 (classSound, 0, U"To Manipulation...", nullptr, 1, NEW_Sound_to_Manipulation);
	praat_addAction1 (classSound, 0, U"Convert -", nullptr, 0, nullptr);
		praat_addAction1 (classSound, 0, U"Convert to mono", nullptr, 1, NEW_Sound_convertToMono);
		praat_addAction1 (classSound, 0, U"Convert to stereo", nullptr, 1, NEW_Sound_convertToStereo);
		praat_addAction1 (classSound, 0, U"Extract all channels", nullptr, 1, NEWMANY_Sound_extractAllChannels);
		praat_addAction1 (classSound, 0, U"Extract one channel...", nullptr, 1, NEW_Sound_extractChannel);
		praat_addAction1 (classSound, 0,   U"Extract left channel", U"*Extract one channel...", praat_DEPTH_1 | praat_DEPRECATED_2010, NEW_Sound_extractLeftChannel);
		praat_addAction1 (classSound, 0,   U"Extract right channel", U"*Extract one channel...", praat_DEPTH_1 | praat_DEPRECATED_2010, NEW_Sound_extractRightChannel);
		praat_addAction1 (classSound, 0, U"Extract part...", nullptr, 1, NEW_Sound_extractPart);
		praat_addAction1 (classSound, 0, U"Extract part for overlap...", nullptr, 1, NEW_Sound_extractPartForOverlap);
		praat_addAction1 (classSound, 0, U"Resample...", nullptr, 1, NEW_Sound_resample);
		praat_addAction1 (classSound, 0, U"-- enhance --", nullptr, 1, nullptr);
		praat_addAction1 (classSound, 0, U"Lengthen (overlap-add)...", nullptr, 1, NEW_Sound_lengthen_overlapAdd);
		praat_addAction1 (classSound, 0,   U"Lengthen (PSOLA)...", U"*Lengthen (overlap-add)...", praat_DEPTH_1 | praat_DEPRECATED_2007, NEW_Sound_lengthen_overlapAdd);
		praat_addAction1 (classSound, 0, U"Deepen band modulation...", nullptr, 1, NEW_Sound_deepenBandModulation);
		praat_addAction1 (classSound, 0, U"-- cast --", nullptr, 1, nullptr);
		praat_addAction1 (classSound, 0, U"Down to Matrix", nullptr, 1, NEW_Sound_to_Matrix);
	praat_addAction1 (classSound, 0, U"Filter -", nullptr, 0, nullptr);
		praat_addAction1 (classSound, 0, U"Filtering tutorial", nullptr, 1, HELP_FilteringTutorial);
		praat_addAction1 (classSound, 0, U"-- frequency-domain filter --", nullptr, 1, nullptr);
		praat_addAction1 (classSound, 0, U"Filter (pass Hann band)...", nullptr, 1, NEW_Sound_filter_passHannBand);
		praat_addAction1 (classSound, 0, U"Filter (stop Hann band)...", nullptr, 1, NEW_Sound_filter_stopHannBand);
		praat_addAction1 (classSound, 0, U"Filter (formula)...", nullptr, 1, NEW_Sound_filter_formula);
		praat_addAction1 (classSound, 0, U"-- time-domain filter --", nullptr, 1, nullptr);
		praat_addAction1 (classSound, 0, U"Filter (one formant)...", nullptr, 1, NEW_Sound_filter_oneFormant);
		praat_addAction1 (classSound, 0, U"Filter (pre-emphasis)...", nullptr, 1, NEW_Sound_filter_preemphasis);
		praat_addAction1 (classSound, 0, U"Filter (de-emphasis)...", nullptr, 1, NEW_Sound_filter_deemphasis);
	praat_addAction1 (classSound, 0, U"Combine -", nullptr, 0, nullptr);
		praat_addAction1 (classSound, 0, U"Combine to stereo", nullptr, 1, NEW1_Sounds_combineToStereo);
		praat_addAction1 (classSound, 0, U"Concatenate", nullptr, 1, NEW1_Sounds_concatenate);
		praat_addAction1 (classSound, 0, U"Concatenate recoverably", nullptr, 1, NEW2_Sounds_concatenateRecoverably);
		praat_addAction1 (classSound, 0, U"Concatenate with overlap...", nullptr, 1, NEW1_Sounds_concatenateWithOverlap);
		praat_addAction1 (classSound, 2, U"Convolve...", nullptr, 1, NEW1_Sounds_convolve);
		praat_addAction1 (classSound, 2,   U"Convolve", U"*Convolve...", praat_DEPTH_1 | praat_DEPRECATED_2010, NEW1_Sounds_convolve_old);
		praat_addAction1 (classSound, 2, U"Cross-correlate...", nullptr, 1, NEW1_Sounds_crossCorrelate);
		praat_addAction1 (classSound, 2, U"To ParamCurve", nullptr, 1, NEW1_Sounds_to_ParamCurve);

	praat_addAction2 (classLongSound, 0, classSound, 0, U"Save as WAV file...", nullptr, 0, SAVE_LongSound_Sound_saveAsWavFile);
	praat_addAction2 (classLongSound, 0, classSound, 0,   U"Write to WAV file...", U"*Save as WAV file...", praat_DEPRECATED_2011, SAVE_LongSound_Sound_saveAsWavFile);
	praat_addAction2 (classLongSound, 0, classSound, 0, U"Save as AIFF file...", nullptr, 0, SAVE_LongSound_Sound_saveAsAiffFile);
	praat_addAction2 (classLongSound, 0, classSound, 0,   U"Write to AIFF file...", U"*Save as AIFF file...", praat_DEPRECATED_2011, SAVE_LongSound_Sound_saveAsAiffFile);
	praat_addAction2 (classLongSound, 0, classSound, 0, U"Save as AIFC file...", nullptr, 0, SAVE_LongSound_Sound_saveAsAifcFile);
	praat_addAction2 (classLongSound, 0, classSound, 0,   U"Write to AIFC file...", U"*Save as AIFC file...", praat_DEPRECATED_2011, SAVE_LongSound_Sound_saveAsAifcFile);
	praat_addAction2 (classLongSound, 0, classSound, 0, U"Save as NeXT/Sun file...", nullptr, 0, SAVE_LongSound_Sound_saveAsNextSunFile);
	praat_addAction2 (classLongSound, 0, classSound, 0,   U"Write to NeXT/Sun file...", U"*Save as NeXT/Sun file...", praat_DEPRECATED_2011, SAVE_LongSound_Sound_saveAsNextSunFile);
	praat_addAction2 (classLongSound, 0, classSound, 0, U"Save as NIST file...", nullptr, 0, SAVE_LongSound_Sound_saveAsNistFile);
	praat_addAction2 (classLongSound, 0, classSound, 0,   U"Write to NIST file...", U"*Save as NIST file...", praat_DEPRECATED_2011, SAVE_LongSound_Sound_saveAsNistFile);
	praat_addAction2 (classLongSound, 0, classSound, 0, U"Save as FLAC file...", nullptr, 0, SAVE_LongSound_Sound_saveAsFlacFile);
	praat_addAction2 (classLongSound, 0, classSound, 0,   U"Write to FLAC file...", U"*Save as FLAC file...", praat_DEPRECATED_2011, SAVE_LongSound_Sound_saveAsFlacFile);
}

/* End of file praat_Sound.cpp */
