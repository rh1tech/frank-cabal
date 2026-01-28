/* Cabal - Legacy Game Implementations
 *
 * Stub AudioCD manager for RP2350 (no CD audio support)
 */

#ifndef BACKENDS_AUDIOCD_RP2350_H
#define BACKENDS_AUDIOCD_RP2350_H

#include "backends/audiocd/audiocd.h"

class RP2350AudioCDManager : public AudioCDManager {
public:
	RP2350AudioCDManager() {}
	virtual ~RP2350AudioCDManager() {}

	virtual bool open() { return true; }
	virtual void close() {}
	virtual bool play(int track, int numLoops, int startFrame, int duration, bool onlyEmulate = false) { return false; }
	virtual bool isPlaying() const { return false; }
	virtual void setVolume(byte volume) {}
	virtual void setBalance(int8 balance) {}
	virtual void stop() {}
	virtual void update() {}
	virtual Status getStatus() const {
		Status status;
		status.playing = false;
		status.track = 0;
		status.start = 0;
		status.duration = 0;
		status.numLoops = 0;
		status.volume = 0;
		status.balance = 0;
		return status;
	}
};

#endif // BACKENDS_AUDIOCD_RP2350_H
