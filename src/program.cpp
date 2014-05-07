#include "program.h"
#include "syntax/process.h"
#include "syntax/operator.h"
#include "syntax/record.h"
#include "syntax/channel.h"
#include "preprocessor.h"

program::program()
{
	// Define the basic types. In this case, 'node' (also known as wire, net, etc)
	types.types.push_back(new keyword("node"));
}

program::~program()
{
}

/**
 */
bool program::parse(tokenizer &tokens)
{
	while (tokens.peek_next() != "")
	{
		tokens.increment();
		tokens.push_expected("[preprocessor]");
		tokens.push_expected("[record]");
		tokens.push_expected("[channel]");
		tokens.push_expected("[process]");
		tokens.push_expected("[operator]");
		tokens.syntax(__FILE__, __LINE__);
		tokens.decrement();

		tokens.increment();
		tokens.push_bound("[preprocessor]");
		tokens.push_bound("[record]");
		tokens.push_bound("[channel]");
		tokens.push_bound("[process]");
		tokens.push_bound("[operator]");
		if (preprocessor::is_next(tokens))
			preprocessor(tokens, *this);
		else if (process::is_next(tokens))
			types.insert(tokens, new process(tokens, types));
		else if (operate::is_next(tokens))
			types.insert(tokens, new operate(tokens, types));
		else if (record::is_next(tokens))
			types.insert(tokens, new record(tokens, types));
		else if (channel::is_next(tokens))
			types.insert(tokens, new channel(tokens, types));

		tokens.decrement();
	}

	loaded.push_back(tokens.files.back().first);

	return true;
}
