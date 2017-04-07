
#include <cstdio>
#include <cassert>
#include <string>
#include <vector>
#include <list>

std::string ev( const std::vector<std::string>& cur_cmd )
{
	int i = 0;
	std::string ret = "<ret of";
	for (auto x : cur_cmd)
	{
		printf( "%s %d: \"%s\"\n", i == 0 ? "eval: " : "      " , i, x.c_str() );
		i++;
		ret += " ";
		ret += x;
	}
	ret += ">";
	return ret;
}


typedef std::vector<std::string> str_list_t;

typedef std::list< str_list_t > str_list_stack_t;

enum {
	mis_normal,
	mis_string,
	mis_string_esc,
	mis_vstring,
};

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

void mi( const std::string& e )
{
	size_t ii = 0;

	int state = mis_normal;
	std::list<int> state_stack;

	std::string cur_str;
	str_list_t cur_cmd;
	str_list_stack_t cmd_stack;

	std::list<char> brace_stack;
	std::list<char> sbrace_stack;

	while (ii <= e.size())
	{
		int ch = ii < e.size() ? e[ii] : -1;
		ii++;

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
					std::string res = ev( cur_cmd );
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
						cur_str = res;
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
			else
			{
				cur_str.push_back( ch );
			}
		}
		else if (state == mis_string)
		{
			if (ch == '"')
			{
				cur_cmd.push_back( cur_str );
				cur_str.clear();

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
		else
		{
			assert( 0 );
		}
	}
}

void test1()
{
	mi( std::string("let a \"string test\"") );
	mi( std::string("let a (mul 2 2) (mul 3 (add 1 1))") );
	mi( std::string("if $a {curly test {test} test} else { not wrong too }") );
}

int main()
{
	test1();
	return 0;
}

// vim:cindent:
