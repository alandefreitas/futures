import os
import re
import xml.etree.ElementTree as ET

def declare_variables(variables, macro):
    @macro
    def code_snippet(filename: str, snippet: str = "", language: str = "", indent = 0, replacements = {}):
        """
        Load code from a file and save as a preformatted code block.
        If a language is specified, it's passed in as a hint for syntax highlighters.

        Example usage in markdown:

            {{code_from_file("code/myfile.py", "python")}}

        """
        docs_dir = variables.get("docs_dir", "docs")

        # Look for file
        abs_docs_path = os.path.abspath(os.path.join(docs_dir, filename))
        abs_root_path = os.path.abspath(os.path.join(docs_dir, "..", filename))
        abs_examples_path = os.path.abspath(os.path.join(docs_dir, "../examples/", filename))
        abs_path = ''
        if os.path.exists(abs_docs_path):
            abs_path = abs_docs_path
        elif os.path.exists(abs_root_path):
            abs_path = abs_root_path
        elif os.path.exists(abs_examples_path):
            abs_path = abs_examples_path

        if language == '':
            extension = os.path.splitext(abs_path)[1]
            if extension == '.txt':
                if abs_path.endswith('CMakeLists.txt'):
                    language = 'cmake'
                else:
                    language = 'text'
            elif extension == '.cmake':
                language = 'cmake'
            elif extension == '.md':
                language = 'markdown'
            else:
                language = 'cpp'

        # File not found
        if not os.path.exists(abs_path):
            return f"""<b>File not found: {filename}</b>"""

        # Read snippet from file
        with open(abs_path, "r") as f:
            if not snippet:
                print(f"Fail (no snippet): {abs_path}\nsnippet: {snippet}\nlanguage: {language}")
                return (
                    f"""```{language}\n{f.read()}\n```"""
                )
            else:
                # Snippet tags
                comment_token = '//'
                if language == 'markdown':
                    comment_token = '<!--'
                elif language != 'cpp':
                    comment_token = '#'
                snippet_open = comment_token + '['
                snippet_close = comment_token + ']'

                # Find the snippet start
                contents = f.read()
                open_token = snippet_open + snippet
                start_pos = contents.find(open_token)
                if start_pos == -1:
                    print(f"Fail (no snippet start): {abs_path}\nsnippet: {snippet}\nlanguage: {language}")
                    return f"""<b>Snippet {open_token} not found in {filename}</b>"""
                start_pos += len(open_token)

                # Find the snippet end
                close_token = snippet_close + snippet
                end_pos = contents.find(close_token, start_pos)
                if end_pos == -1:
                    # try only the close token, without snippet name
                    # (the close_token is for internal comments)
                    end_pos = contents.find(snippet_close, start_pos)
                if end_pos == -1:
                    print(f"Fail (no snippet end): {abs_path}\nsnippet: {snippet}\nlanguage: {language}")
                    return f"""<b>Snippet {snippet} not found in {filename}</b>"""
                contents = contents[start_pos:end_pos]

                # Find snippet header, if any
                content_lines = contents.splitlines()
                first_line = content_lines[0]
                header = ''
                if not first_line.isspace() and first_line:
                    header = first_line.strip()

                # Identify snippet indent
                indent_size = 20
                for line in content_lines[1:]:
                    if not line.isspace() and line:
                        first_char_pos = len(line) - len(line.lstrip())
                        indent_size = min(indent_size, first_char_pos)

                # Open code block
                contents = ''
                is_code = language not in ['markdown', 'text']
                if is_code:
                    if header:
                        contents += '=== "' + header + '"\n\n    '
                        contents += ' ' * indent
                    contents += '```' + language
                    if len(content_lines) > 10:
                        contents += ' linenums="1" '
                    contents += '\n'
                    contents += ' ' * indent

                # Fill block with appropriate indentation
                for line in content_lines[1:]:
                    line_is_comment = \
                        line.lstrip().startswith(snippet_open) or \
                        line.lstrip().startswith(snippet_close)
                    if line_is_comment:
                        continue
                    is_in_content_tab = is_code and header
                    if is_in_content_tab:
                        contents += '    '
                    contents += line[indent_size:] + '\n'
                    contents += ' ' * indent

                # Close code block
                if is_code:
                    if header:
                        contents += '    '
                    contents += '```\n'

                for [original, replace] in replacements.items():
                    contents = contents.replace(original, replace)

                return contents

    @macro
    def all_code_snippets(filename: str, language: str = "cpp"):
        """
        Load code from a file and save as a preformatted code block.
        If a language is specified, it's passed in as a hint for syntax highlighters.

        Example usage in markdown:

            {{all_code_snippets("code/myfile.py", "python")}}

        """
        docs_dir = variables.get("docs_dir", "docs")

        # Look for file
        abs_docs_path = os.path.abspath(os.path.join(docs_dir, filename))
        abs_root_path = os.path.abspath(os.path.join(docs_dir, "..", filename))
        abs_examples_path = os.path.abspath(os.path.join(docs_dir, "../examples/", filename))
        abs_path = ''
        if os.path.exists(abs_docs_path):
            abs_path = abs_docs_path
        elif os.path.exists(abs_root_path):
            abs_path = abs_root_path
        elif os.path.exists(abs_examples_path):
            abs_path = abs_examples_path

        # File not found
        if not os.path.exists(abs_path):
            return f"""<b>File not found: {filename}</b>"""

        # Read all snippets from file
        with open(abs_path, "r") as f:
            # Snippet tags
            snippet_open = '//[' if language == 'cpp' else '#['
            snippet_close = '//]' if language == 'cpp' else '#]'

            # Extract the snippet names
            contents = f.read()
            snippets = []
            start_pos = contents.find(snippet_open)
            while start_pos != -1:
                name_start = start_pos + len(snippet_open)
                name_end = contents.find(' ', name_start)
                if name_end != -1:
                    snippets.append(contents[name_start:name_end])
                start_pos = contents.find(snippet_open, name_end)

            # Render all snippets
            contents = ''
            for snippet in snippets:
                contents += code_snippet(filename, snippet, language)
                contents += '\n'

            return contents

    def markdownify(node: ET.Element):
        wrap_with = {
            'ref': '`',
            'computeroutput': '`',
            'title': '**',
        }
        begin_with = {
            'listitem': '* ',
            'ulink': '[',
        }
        end_with = {
            'title': '\n\n',
            'ulink': ']',
        }
        text = ''
        if node.tag in begin_with:
            text = begin_with[node.tag]
        if node.tag in wrap_with:
            text += wrap_with[node.tag]
        if node.text:
            text += node.text
        if node.tag in wrap_with:
            text += wrap_with[node.tag]
        if node.tag in end_with:
            text += end_with[node.tag]
        if node.get('url'):
            text += '(' + node.get('url') + ')'
        for c in node:
            text += markdownify(c)
        if node.tail:
            text += node.tail
        return text

    @macro
    def doxygen_cpp_macros_table(filename: str):
        """
        Load C++ macros from a xml file generated with doxygen and document all macros.
        """
        docs_dir = variables.get("docs_dir", "docs")

        # Look for file
        xml_path = os.path.abspath(os.path.join(docs_dir, 'xml', filename))
        if not os.path.exists(xml_path):
            return f'{xml_path} does NOT exist'

        tree = ET.parse(xml_path)
        root = tree.getroot()
        defs = root[0]
        brief = defs.find('briefdescription')

        # Generate table
        res = '| Option     | Description       |\n'
        res += '|------------|-------------------|\n'
        for section in defs.findall('sectiondef'):
            for member in section.findall('memberdef'):
                res += f'|[`{member.find("name").text}`](#{member.find("name").text.lower()})|'
                brief = member.find('briefdescription')
                for para in brief:
                    res += markdownify(para).replace("\n", "")
                res += f'|\n'
        res += f'\n\n'
        return res

    @macro
    def doxygen_cpp_macros(filename: str):
        """
        Load C++ macros from a xml file generated with doxygen and document all macros.
        """
        docs_dir = variables.get("docs_dir", "docs")

        # Look for file
        xml_path = os.path.abspath(os.path.join(docs_dir, 'xml', filename))
        if not os.path.exists(xml_path):
            return f'{xml_path} does NOT exist'

        tree = ET.parse(xml_path)
        root = tree.getroot()
        defs = root[0]
        brief = defs.find('briefdescription')

        res = ''
        for para in brief:
            res += '>' + markdownify(para) + '\n>\n'
        res += '\n\n'
        details = defs.find('detaileddescription')
        for para in details:
            res += markdownify(para) + '\n\n'
        res += '\n\n'
        for section in defs.findall('sectiondef'):
            for member in section.findall('memberdef'):
                res += f'### {member.find("name").text}\n\n'
                res += f'```cpp\n'
                res += f'#define {member.find("name").text}\n\n'
                res += f'```\n\n'
                brief = member.find('briefdescription')
                for para in brief:
                    res += markdownify(para) + '\n\n'
                res += f'**Description**\n\n'
                details = member.find('detaileddescription')
                for para in details:
                    res += markdownify(para) + '\n\n'
        return res

    @macro
    def cmake_options(filename: str):
        """
        Render options from a CMakelists file
        """
        docs_dir = variables.get("docs_dir", "docs")

        # Look for file
        abs_root_path = os.path.abspath(os.path.join(docs_dir, "..", filename))
        abs_examples_path = os.path.abspath(os.path.join(docs_dir, "../examples/", filename))
        abs_path = ''
        if os.path.exists(abs_root_path):
            abs_path = abs_root_path
        elif os.path.exists(abs_examples_path):
            abs_path = abs_examples_path

        language = 'cmake'

        # File not found
        if not os.path.exists(abs_path):
            return f"""<b>File not found: {filename}</b>"""

        def sanitize_default_value(str):
            if str == '${MASTER_PROJECT}':
                return '`ON` if running CMake from the root directory'
            if str == '${FUTURES_NOT_CROSSCOMPILING}':
                return '`ON` if not crosscompiling'
            if str == '${DEBUG_MODE}':
                return '`ON` if compiling in Debug mode'
            if str == '${NOT_MSVC}':
                return '`ON` if not compiling with MSVC'
            return '`' + str + '`'

        # Read options from file
        with open(abs_path, "r") as f:
            contents = f.read()

            res =  '| Option     | Description       | Default     |\n'
            res += '|------------|-------------------|-------------|\n'

            pattern = re.compile(" *option\\( *([^ ]+) *\"([^\"]+)\" *(.+) *\\).*")
            for line in contents.splitlines():
                result = pattern.search(line)
                if result:
                    res += f'|`{result.group(1)}`|{result.group(2)}|{sanitize_default_value(result.group(3))}|\n'
            return res