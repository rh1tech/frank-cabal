/* Cabal - Legacy Game Implementations
 *
 * Filesystem factory for RP2350
 */

#ifndef BACKENDS_FS_RP2350_FS_FACTORY_H
#define BACKENDS_FS_RP2350_FS_FACTORY_H

#include "backends/fs/fs-factory.h"
#include "common/singleton.h"

/**
 * Creates RP2350FileSystemNode objects.
 */
class RP2350FilesystemFactory : public FilesystemFactory, public Common::Singleton<RP2350FilesystemFactory> {
public:
	virtual AbstractFSNode *makeRootFileNode() const;
	virtual AbstractFSNode *makeCurrentDirectoryFileNode() const;
	virtual AbstractFSNode *makeFileNodePath(const Common::String &path) const;

protected:
	RP2350FilesystemFactory() {}

private:
	friend class Common::Singleton<SingletonBaseType>;
};

#endif // BACKENDS_FS_RP2350_FS_FACTORY_H
