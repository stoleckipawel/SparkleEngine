#include "SparkleCli.h"

#include "SparkleCliArguments.h"
#include "SparkleCliDispatcher.h"
#include "SparkleCliOutput.h"
#include "SparkleCliParser.h"

#include <ostream>

namespace SparkleLauncher
{
	int SparkleCli::Run(int argc, char** argv, std::ostream& output, std::ostream& error) const
	{
		const SparkleCliOutput cliOutput;
		const SparkleCliParser parser;
		SparkleCliArguments arguments;
		if (!parser.Parse(argc, argv, arguments, error))
		{
			cliOutput.PrintUsage(error);
			return 1;
		}

		if (arguments.ShowHelp)
		{
			cliOutput.PrintUsage(output);
			return 0;
		}

		if (arguments.ListOperations)
		{
			cliOutput.PrintOperationList(output);
			return 0;
		}

		if (arguments.OperationId.empty())
		{
			error << "Sparkle: operation id is required.\n";
			cliOutput.PrintUsage(error);
			return 1;
		}

		const SparkleCliDispatcher dispatcher;
		return dispatcher.Dispatch(arguments, output, error);
	}
}