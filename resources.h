#pragma once

/** A general class that other classes can subclass, to allow things like
*   easier memory management.
*/

using std::vector;

namespace Gigalomania {
	class TrackedObject {
		static vector<TrackedObject *> tags;
		size_t tag;
		int deleteLevel;

	public:
		TrackedObject();
		virtual ~TrackedObject();

		static void initialise();
		static void flushAll();
		static void flush(int deleteLevel);
		static void cleanup();
		static size_t addTag(TrackedObject *ptr);
		static void removeTag(size_t tag);
		static size_t getNumTags();
		static TrackedObject *getTag(size_t index);
		static TrackedObject *ptrFromTag(size_t tag);
		virtual const char *getClass() const=0;
		bool isClass(const char *classname) const;
	};
}
