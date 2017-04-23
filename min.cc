
#include <cstdio>
#include <cassert>
#include <string>
#include <vector>
#include <list>
#include <map>

typedef std::vector<std::string> str_list_t;

typedef std::list< str_list_t > str_list_stack_t;

enum {
	mis_normal = 0,
	mis_string = 1,
	mis_string_esc = 2,
	mis_vstring    = 3,
	mis_variable   = 4,
};

enum {
	mi_ev_ok = 0,
	mi_ev_error,
	mi_ev_no_proc,
	mi_ev_return,	// special case: return from function
	mi_ev_do,	// special case: eval one of the args
};

struct mi_uproc {
	std::string name;
	std::string body;
	std::list< std::pair<std::string, std::string> > args;
};

struct mirtc {
	mirtc *parent_rtc;

	typedef std::map< std::string, mi_uproc > proc_table_t;
	proc_table_t proc_table;

	typedef std::map< std::string, std::string > var_table_t;
	var_table_t var_table;

	std::string side_effect;
};

static char decode_esc_char( char ch );

void mi( mirtc *parent_rtc, const std::string& e );

typedef int (*mi_command_t)( mirtc *rtc, const std::vector<std::string>& cur_cmd, std::string *res );



mirtc *mi_find_var_rtc( mirtc *rtc, const std::string& var_name, std::string **ppval )
{
	while (rtc) {
		auto x = rtc->var_table.find( var_name );
		if (x != rtc->var_table.end()) {
			if (ppval) *ppval = &(x->second);
			return rtc;
		}
		rtc = rtc->parent_rtc;
	}
	return NULL;
}

mirtc *mi_find_proc_rtc( mirtc *rtc, const std::string& proc_name, mi_uproc **ppuproc )
{
	while (rtc) {
		auto x = rtc->proc_table.find( proc_name );
		if (x != rtc->proc_table.end()) {
			if (ppuproc) *ppuproc = &(x->second);
			return rtc;
		}
		rtc = rtc->parent_rtc;
	}
	return NULL;
}

static std::string mi_val_join( const std::vector<std::string>& list, unsigned begin, unsigned end = -1u )
{
	if (end == -1u)
		end = list.size();

	std::string ret;
	bool first = true;

	for (unsigned i = begin; i<end; i++)
	{
		if (!first)
		{
			ret += " ";
			first = false;
		}
		ret += list[i];
	}

	return ret;
}


int micm_var( mirtc *rtc, const std::vector<std::string>& cur_cmd, std::string *res );

// assign (create if not available) variable: let varname value 
int micm_let( mirtc *rtc, const std::vector<std::string>& cur_cmd, std::string *res )
{
	if (cur_cmd.size() >= 2)
	{
		mirtc *vrtc;

		vrtc = mi_find_var_rtc( rtc, cur_cmd[1], NULL );

		return micm_var( vrtc ? vrtc : rtc, cur_cmd, res );
	}
	else
	{
		return mi_ev_error;
	}
	return mi_ev_ok;
}

// create variable: var varname value
int micm_var( mirtc *rtc, const std::vector<std::string>& cur_cmd, std::string *res )
{
	if (cur_cmd.size() >= 2)
	{
		std::string v;
		v = mi_val_join( cur_cmd, 2 );
		rtc->var_table[ cur_cmd[1] ] = v;
		rtc->side_effect = v;
		if (res) *res = v;
		printf( "DBG: create var: [%s], value [%s]\n", cur_cmd[1].c_str(), v.c_str() );
	}
	else
	{
		return mi_ev_error;
	}
	return mi_ev_ok;
}

// proc hello name {print "hello, " $name "\n"}
int micm_proc( mirtc *rtc, const std::vector<std::string>& cur_cmd, std::string *res )
{
	mi_uproc proc;
	if (cur_cmd.size() >= 3)
	{
		proc.name = cur_cmd[1];
		proc.body = cur_cmd.back();
		for (unsigned i = 2; i<cur_cmd.size()-1; i++)
		{
			proc.args.push_back( std::make_pair(cur_cmd[i], std::string()) );
		}

		rtc->proc_table[ proc.name ] = proc;

		if (res) *res = cur_cmd[1];
	}
	else
	{
		return mi_ev_error;
	}
	return mi_ev_ok;
}

int micm_if( mirtc *rtc, const std::vector<std::string>& cur_cmd, std::string *res )
{
	bool yes = false;
	bool OK = false;
	if (cur_cmd.size() >= 2)
	{
		if (cur_cmd[1].size()>0 && strtol( cur_cmd[1].c_str(), NULL, 0 ) != 0)
		{
			yes = true;
		}
	}

	if (cur_cmd.size() == 3)
	{
		// if bool expr-true
		if (yes)
		{
			mi( rtc, cur_cmd[2] );
			OK = true;
		}
	}
	else if (cur_cmd.size() == 4)
	{
		// if bool expr-true expr-false
		mi( rtc, yes ? cur_cmd[2] : cur_cmd[3] );
		OK = true;
	}
	else if (cur_cmd.size() == 5 && cur_cmd[3].compare("else") == 0)
	{
		// if bool expr-true else expr-false
		mi( rtc, yes ? cur_cmd[2] : cur_cmd[4] );
		OK = true;
	}
	else
	{
		return mi_ev_error;
	}

	if (OK)
	{
		if (res) *res = rtc->side_effect;
	}

	return mi_ev_ok;
}

//
// function library
//

int micm_add( mirtc *rtc, const std::vector<std::string>& cur_cmd, std::string *res )
{
	if (cur_cmd.size() >= 2)
	{
		long ret = 0;
		char buf1[64];

		for (unsigned i = 1; i<cur_cmd.size(); i++)
		{
			ret += strtol( cur_cmd[i].c_str(), NULL, 0 );
		}

		snprintf( buf1, sizeof(buf1), "%ld", ret );

		printf( "DBG: add ==> %s\n", buf1 );

		if (res) *res = std::string( buf1 );
	}
	else
	{
		return mi_ev_error;
	}
	return mi_ev_ok;
}

int micm_mul( mirtc *rtc, const std::vector<std::string>& cur_cmd, std::string *res )
{
	if (cur_cmd.size() >= 2)
	{
		long ret = 1;
		char buf1[64];

		for (unsigned i = 1; i<cur_cmd.size(); i++)
		{
			ret *= strtol( cur_cmd[i].c_str(), NULL, 0 );
		}

		snprintf( buf1, sizeof(buf1), "%ld", ret );

		printf( "DBG: mul ==> %s\n", buf1 );

		if (res) *res = std::string( buf1 );
	}
	else
	{
		return mi_ev_error;
	}
	return mi_ev_ok;
}

int micm_eq( mirtc *rtc, const std::vector<std::string>& cur_cmd, std::string *res )
{
	if (cur_cmd.size() >= 3)
	{
		bool OK = true;

		for (unsigned i = 2; i<cur_cmd.size(); i++)
		{
			if (cur_cmd[1] != cur_cmd[i])
			{
				OK = false;
			}
		}

		if (res) *res = OK ? std::string( "1" ) : std::string();
	}
	else
	{
		return mi_ev_error;
	}
	return mi_ev_ok;
}

int micm_print( mirtc *rtc, const std::vector<std::string>& cur_cmd, std::string *res )
{
	bool first = true;
	for (unsigned i = 1; i<cur_cmd.size(); i++)
	{
		if (!first)
		{
			printf( " " );
		}
		printf( "%s", cur_cmd[i].c_str() );
		first = false;
	}
	if (res) res->clear();
	return mi_ev_ok;
}

int micm_println( mirtc *rtc, const std::vector<std::string>& cur_cmd, std::string *res )
{
	micm_print( rtc, cur_cmd, res );
	printf( "\n" );
	return mi_ev_ok;
}


struct micm_builtin_table_row {
	const char *name;
	mi_command_t entry;
};
static micm_builtin_table_row micm_builtin_table[] = {
	{ "proc", micm_proc },
	{ "var", micm_var },
	{ "let", micm_let },
	{ "add", micm_add },
	{ "mul", micm_mul },
	{ "eq", micm_eq },
	{ "if", micm_if },
	{ "print", micm_print },
	{ "println", micm_println },
};

int mi_call_uproc( mirtc *parent_rtc, const std::vector<std::string>& cur_cmd, std::string *res )
{
	mirtc rtc;

	rtc.parent_rtc = parent_rtc;

	// set args into variables

	// call the mi() on the proc body

	mi_uproc *puproc = NULL;
	mirtc *prtc = mi_find_proc_rtc( parent_rtc, cur_cmd[0], &puproc );
	if (prtc) {
		unsigned i = 1;
		for (auto I : puproc->args ) {
			if (i >= cur_cmd.size())
				break;
			printf( "call_proc [%s]: local var: [%s] = [%s]\n",
					cur_cmd[0].c_str(), I.first.c_str(), cur_cmd[i].c_str() );

			std::vector<std::string> var_cmd;
			var_cmd.push_back( std::string() );
			var_cmd.push_back( I.first );
			var_cmd.push_back( cur_cmd[i] );
			micm_var( &rtc, var_cmd, NULL );
			i++;
		}
		mi( &rtc, puproc->body );
		if (res) *res = rtc.side_effect;
	}
	else {
		return mi_ev_no_proc;
	}

	return mi_ev_ok;
}

int mi_call( mirtc *rtc, const std::vector<std::string>& cur_cmd, std::string *res )
{
#if 1
	int i = 0;
	for (auto x : cur_cmd)
	{
		printf( "%s %d: \"%s\"\n", i == 0 ? "eval: " : "      " , i, x.c_str() );
		i++;
	}
#endif

	int ret = mi_call_uproc( rtc, cur_cmd, res );
	
	if (ret == mi_ev_no_proc)
	{
		for (auto x : micm_builtin_table)
		{
			if ( cur_cmd[0].compare(x.name) == 0 )
			{
				return x.entry( rtc, cur_cmd, res );
			}
		}
	}

	printf( "ERR: unk proc: [%s]\n", cur_cmd[0].c_str() );

	assert( false );

	return mi_ev_ok;
}


static char decode_esc_char( char ch )
{
	switch (ch)
	{
	case 'n': return '\n';
	case 'r': return '\r';
	case 't': return '\t';
	case 'e': return 27;
	default: return ch;
	}
}

void mi( mirtc *parent_rtc, const std::string& e )
{
	size_t ii = 0;

	mirtc rtc;

	int state = mis_normal;
	std::list<int> state_stack;

	std::string side_effect;

	std::string cur_str;
	str_list_t cur_cmd;
	str_list_stack_t cmd_stack;

	std::list<char> brace_stack;
	std::list<char> sbrace_stack;

	rtc.parent_rtc = parent_rtc;

	while (ii <= e.size())
	{
		int ch = ii < e.size() ? e[ii] : -1;
		ii++;

		//printf( "state: %d, char %c (%d)\n", state, isprint(ch) ? ch : '.', ch );

		if (state == mis_normal)
		{
			//if (ch == '\\') { } else
			if (ch == -1 || ch == ';' || ch == '\n' || (not(brace_stack.empty()) and brace_stack.back() == ch))
			{
				if (not cur_str.empty())
				{
					cur_cmd.push_back( cur_str );
					cur_str.clear();
				}
				if (not cur_cmd.empty())
				{
					std::string res;
					int ret;

					// return is not a function. (for now?)
					if (cur_cmd[0].compare( "return" ) == 0)
					{
						parent_rtc->side_effect = mi_val_join( cur_cmd, 1 );
						printf( "DBG: return %s\n", parent_rtc->side_effect.c_str() );
						return;
					}

					ret = mi_call( &rtc, cur_cmd, &res );
					// if (ret == )

					cur_cmd.clear();

					if (not(cmd_stack.empty()) and not(brace_stack.empty()) and brace_stack.back() == ch )
					{
						cur_cmd = cmd_stack.back();
						cmd_stack.pop_back();

						cur_cmd.push_back( res );

						brace_stack.pop_back();
					}
					else
					{
						//cur_str = res;
						side_effect = res;
					}
				}
			}
			else if (isblank(ch))
			{
				if (not cur_str.empty())
				{
					cur_cmd.push_back( cur_str );
					cur_str.clear();
				}
			}
			else if (ch == '(')
			{
				brace_stack.push_back( ')' );

				if (not cur_str.empty())
				{
					cur_cmd.push_back( cur_str );
					cur_str.clear();
				}

				cmd_stack.push_back( cur_cmd );
				cur_cmd.clear();
			}
			else if (ch == '"')
			{
				if (not cur_str.empty())
				{
					cur_cmd.push_back( cur_str );
					cur_str.clear();
				}
				//sbrace_stack.push_back( '"' );
				state_stack.push_back( state );
				state = mis_string;
			}
			else if (ch == '{')
			{
				if (not cur_str.empty())
				{
					cur_cmd.push_back( cur_str );
					cur_str.clear();
				}
				sbrace_stack.push_back( '}' );
				state_stack.push_back( state );
				state = mis_vstring;
			}
			else if (ch == '$')
			{
				// variable subsitution
				if (not cur_str.empty())
				{
					cur_cmd.push_back( cur_str );
					cur_str.clear();
				}
				state_stack.push_back( state );
				state = mis_variable;
			}
			else
			{
				cur_str.push_back( ch );
			}
		}
		else if (state == mis_string)
		{
			if (ch == '"')
			{
				if (sbrace_stack.empty())
				{
					cur_cmd.push_back( cur_str );
					cur_str.clear();
				}
				else
				{
					cur_str.push_back( ch );
				}

				state = state_stack.back();
				state_stack.pop_back();
			}
			else if (ch == '\\')
			{
				state = mis_string_esc;
			}
			else
			{
				cur_str.push_back( ch );
			}
		}
		else if (state == mis_string_esc)
		{
			cur_str.push_back( decode_esc_char(ch) );
			state = mis_string;
		}
		else if (state == mis_vstring)
		{
			if (ch == sbrace_stack.back())
			{
				state = state_stack.back();
				state_stack.pop_back();

				sbrace_stack.pop_back();

				if (sbrace_stack.empty())
				{
					assert( state != mis_vstring );
					cur_cmd.push_back( cur_str );
					cur_str.clear();
				}
				else
				{
					cur_str.push_back( ch );
				}
			}
			else if (ch == '"')
			{
				cur_str.push_back( ch );

				state_stack.push_back( state );
				state = mis_string;
			}
			else if (ch == '{')
			{
				cur_str.push_back( ch );

				sbrace_stack.push_back( '}' );
				state_stack.push_back( state );
			}
			else
			{
				cur_str.push_back( ch );
			}
		}
		else if (state == mis_variable)
		{
			if (isalnum(ch) || ch == '_')
			{
				cur_str.push_back(ch);
			}
			else
			{
				std::string val;
				/*
				auto x = rtc.var_table.find( cur_str );
				if (x != rtc.var_table.end())
				{
					val = x->second;
				}
				*/

				std::string *pval = NULL;
				mirtc *vrtc;
				vrtc = mi_find_var_rtc( &rtc, cur_str, &pval );
				if (!vrtc)
					pval = &val;

				printf( "DBG: $ var name: [%s] == [%s]\n", cur_str.c_str(), pval->c_str() );

				cur_cmd.push_back( *pval );
				cur_str.clear();
				
				state = state_stack.back();
				state_stack.pop_back();
			}
		}
		else
		{
			printf( "ERR: bad state == %d\n", state );
			assert( false && "bad state" );
		}
	}
}

void test1()
{
	/*
	const char *x1 = 
		"switch (a 1 2) {\n"
		"case \"a\" { print $a }\n"
		"case \"b\" { print $b }\n"
		"case \"c\" { print $c }\n"
		"}\n"
		;
	printf( "raw: [[[\n%s]]]\n", x1 );
	mi( NULL, std::string( x1 ) );
	printf( "-------------------------------\n" );
	mi( NULL, std::string("let a \"string test\"") );
	printf( "-------------------------------\n" );
	mi( NULL, std::string("let a (add (mul 2 2) (mul 3 (add 1 1)))") );
	printf( "-------------------------------\n" );
	mi( NULL, std::string("if $a {curly test {test} test} else { not wrong too }") );
	printf( "-------------------------------\n" );
	mi( NULL, std::string("proc fact n { let a (fact (add n )) }") );
	printf( "-------------------------------\n" );
	*/
	const char *x2 =
		"proc fact n {"					"\n"
		"	if (eq $n 0) {"				"\n"
		"		return 1"			"\n"
		"	} else {"				"\n"
		"		var m (fact (add $n -1))"	"\n"
		"		return (mul $n $m)"		"\n"
		"	}"					"\n"
		"}"						"\n"
		"print (fact 5)"				"\n"
		;
	printf( "raw: [[[\n%s]]]\n", x2 );
	mi( NULL, std::string( x2 ) );

}

int main()
{
	test1();
	return 0;
}

// vim:cindent:
