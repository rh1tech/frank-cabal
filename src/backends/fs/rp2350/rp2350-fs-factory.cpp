/* Cabal - Legacy Game Implementations
 *
 * Filesystem factory for RP2350
 */

#if defined(__RP2350__) || defined(PICO_ON_DEVICE)

#include "backends/fs/rp2350/rp2350-fs-factory.h"
#include "backends/fs/rp2350/rp2350-fs.h"

namespace Common {
DECLARE_SINGLETON(RP2350FilesystemFactory);
}

AbstractFSNode *RP2350FilesystemFactory::makeRootFileNode() const {
	return new RP2350::RP2350FileSystemNode();
}

AbstractFSNode *RP2350FilesystemFactory::makeCurrentDirectoryFileNode() const {
	// On RP2350, we use /cabal as the current directory
	return new RP2350::RP2350FileSystemNode("/cabal");
}

AbstractFSNode *RP2350FilesystemFactory::makeFileNodePath(const Common::String &path) const {
	return new RP2350::RP2350FileSystemNode(path);
}

#endif // __RP2350__ || PICO_ON_DEVICE
