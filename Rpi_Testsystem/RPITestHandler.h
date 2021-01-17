#ifndef RPITESTHANDLER_H
#define RPITESTHANDLER_H

#include <vector>
#include "RPIUnderTest.h"

enum eState
{
	INIT,
	RUN, 
	ERROR
};

constexpr int updateRateMs = 100;
constexpr int colWidth = 17;

class RPITestHandler
{
public:
	RPITestHandler() = delete;
	RPITestHandler(int const& nrOfSystemsUnderTest);

	void startObserving();

private:
	eState mState;
	int mNrOfSystemsUnderTest;
	std::vector<RPIUnderTest> mVecRPIUnderTest;

	void initHandler();
	void printInfo() const;

	template <typename In>
	void printInfoLine(In const& col1, In const& col2, In const& col3, In const& col4) const;
};

#endif
