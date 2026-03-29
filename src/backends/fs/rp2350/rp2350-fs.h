/* Cabal - Legacy Game Implementations
 *
 * Filesystem implementation for RP2350 using cabal_fs
 */

#ifndef BACKENDS_FS_RP2350_FS_H
#define BACKENDS_FS_RP2350_FS_H

#include "backends/fs/abstract-fs.h"
#include "common/stream.h"

namespace RP2350 {

/**
 * Implementation of AbstractFSNode for RP2350 using cabal_fs
 */
class RP2350FileSystemNode : public AbstractFSNode {
protected:
	Common::String _displayName;
	Common::String _path;
	bool _isDirectory;
	bool _isValid;

public:
	/**
	 * Creates a RP2350FileSystemNode for the root of the filesystem.
	 */
	RP2350FileSystemNode();

	/**
	 * Creates a RP2350FileSystemNode for a given path.
	 *
	 * @param path String with the path the new node should point to.
	 */
	RP2350FileSystemNode(const Common::String &path);

	/**
	 * Creates a RP2350FileSystemNode for a given path.
	 *
	 * @param path String with the path the new node should point to.
	 * @param isDir Whether this node is a directory.
	 */
	RP2350FileSystemNode(const Common::String &path, bool isDir);

	virtual bool exists() const { return _isValid; }
	virtual Common::String getDisplayName() const { return _displayName; }
	virtual Common::String getName() const { return _displayName; }
	virtual Common::String getPath() const { return _path; }
	virtual bool isDirectory() const { return _isDirectory; }
	virtual bool isReadable() const { return _isValid; }
	virtual bool isWritable() const { return true; }

	virtual AbstractFSNode *getChild(const Common::String &n) const;
	virtual bool getChildren(AbstractFSList &list, ListMode mode, bool hidden) const;
	virtual AbstractFSNode *getParent() const;

	virtual Common::SeekableReadStream *createReadStream();
	virtual Common::WriteStream *createWriteStream();
};

/**
 * Implementation of SeekableReadStream for RP2350 using cabal_fs
 */
class RP2350FileStream : public Common::SeekableReadStream {
protected:
	void *_handle;  // CabalFile*
	bool _eos;
	bool _err;
	int32 _size;

	// Read buffer for small reads (avoids per-byte SD card overhead)
	static const int kBufSize = 4096;
	byte _buf[kBufSize];
	int32 _bufPos;    // Current read position within buffer
	int32 _bufLen;    // Valid bytes in buffer
	int32 _bufStart;  // File offset where buffer starts

	RP2350FileStream(void *handle);

public:
	virtual ~RP2350FileStream();

	virtual bool err() const { return _err; }
	virtual void clearErr() { _eos = false; _err = false; }
	virtual bool eos() const { return _eos; }

	virtual uint32 read(void *dataPtr, uint32 dataSize);
	virtual int32 pos() const;
	virtual int32 size() const { return _size; }
	virtual bool seek(int32 offs, int whence = SEEK_SET);

	/**
	 * Create a stream from a path.
	 * @param path The path to open
	 * @return The stream, or NULL on failure
	 */
	static RP2350FileStream *makeFromPath(const Common::String &path);
};

/**
 * Implementation of WriteStream for RP2350 using cabal_fs
 */
class RP2350WriteStream : public Common::WriteStream {
protected:
	void *_handle;  // CabalFile*
	bool _err;

	RP2350WriteStream(void *handle);

public:
	virtual ~RP2350WriteStream();

	virtual bool err() const { return _err; }
	virtual void clearErr() { _err = false; }

	virtual uint32 write(const void *dataPtr, uint32 dataSize);
	virtual bool flush();

	/**
	 * Create a write stream from a path.
	 * @param path The path to create/open for writing
	 * @return The stream, or NULL on failure
	 */
	static RP2350WriteStream *makeFromPath(const Common::String &path);
};

} // namespace RP2350

#endif // BACKENDS_FS_RP2350_FS_H
