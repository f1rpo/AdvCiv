#!/usr/bin/env python3
# AI, UI, logging, or other modifications first developed in AdvCiv-SAS (Simple Advanced Strategy)
# (c) 2026 wonderingabout & AI/LLM helpers (see Authors in AdvCiv-SAS's root README.md)

# <!-- custom: The visual refactor exposed both an undefined namespace and a valid module with a removed re-export; syntax checks cannot validate either API contract reliably through Civ4 star imports.
# Read declarations and resource data without importing gameplay modules or emulating Civ4; ast supplies bindings/calls, dataclasses hold partial surfaces, ElementTree reads resource identities, and Git selects affected consumers. (GPT-6.1-Sol) -->
import argparse
import ast
from dataclasses import dataclass, field
from pathlib import Path
import re
# <!-- custom: tokenize and StringIO read actual comment tokens for narrowly justified direct-resource exceptions; marker-like strings must not suppress a lookup failure. (GPT-6.1-Sol) -->
import tokenize
from io import StringIO
import sys
import warnings
import xml.etree.ElementTree as ET

# <!-- custom: Share Git range resolution with the existing source checks rather than giving the API checker a different push/PR comparison policy. (GPT-6.1-Sol) -->
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "lib"))
from git_context import ensure_base, run_git

ROOT = Path(__file__).resolve().parents[4]
PROVIDERS = ("SASVisualUtils", "SASTextScale")


@dataclass
class Surface:
    members: dict = field(default_factory=dict)
    signature: object = None
    opaque: bool = False


UNKNOWN = Surface(opaque=True)
SCALAR = Surface()


def parse(source, filename="<source>"):
    with warnings.catch_warnings():
        warnings.simplefilter("ignore", SyntaxWarning)
        return ast.parse(source, filename=filename)


def read(path):
    raw = path.read_bytes()
    for encoding in ("utf-8-sig", "cp1252"):
        try:
            return raw.decode(encoding)
        except UnicodeDecodeError:
            pass
    return raw.decode("latin-1")


def names(target):
    return [node.id for node in ast.walk(target) if isinstance(node, ast.Name) and isinstance(node.ctx, ast.Store)]


def chain(node):
    if isinstance(node, ast.Name):
        return [node.id]
    if isinstance(node, ast.Attribute):
        parent = chain(node.value)
        return parent + [node.attr] if parent else []
    return []


def bindings(tree):
    result = {}
    for node in ast.walk(tree):
        if isinstance(node, ast.Import):
            for item in node.names:
                result[item.asname or item.name.split(".")[0]] = (item.name, None)
        elif isinstance(node, ast.ImportFrom):
            for item in node.names:
                if item.name != "*":
                    result[item.asname or item.name] = (node.module, item.name)
    return result


def function_surface(node, bound=False):
    args = node.args
    positional = [arg.arg for arg in args.posonlyargs + args.args]
    required = len(positional) - len(args.defaults)
    if bound and positional:
        positional = positional[1:]
        required = max(0, required - 1)
    return Surface(signature=(positional, required, args.vararg is not None, args.kwarg is not None))


# <!-- custom: Dynamic names come from the runtime's own enums, named maps and XML, rather than a duplicate list of allowed collection/member spellings.
# Unknown computed values remain opaque; do not invent members or execute a provider to discover them. (GPT-6.1-Sol) -->
class Catalog:
    def __init__(self, sources, xml_names=None, enums=None):
        self.sources = sources
        self.trees = {name: parse(source, name) for name, source in sources.items()}
        self.xml_names = xml_names or {}
        self.enums = enums or {}
        self.cache = {}
        self.classes = {}
        for module, tree in self.trees.items():
            for node in tree.body:
                if isinstance(node, ast.ClassDef):
                    self.classes[module, node.name] = node

    def literal_names(self, node, env):
        if isinstance(node, ast.Name):
            return env.get(node.id, set())
        if isinstance(node, ast.Attribute) and isinstance(node.value, ast.Name) and node.value.id == "self":
            return env.get(node.attr, set())
        if isinstance(node, ast.Dict):
            return {key.value for key in node.keys if isinstance(key, ast.Constant) and isinstance(key.value, str)}
        if isinstance(node, (ast.Tuple, ast.List, ast.Set)):
            return {item.value for item in node.elts if isinstance(item, ast.Constant) and isinstance(item.value, str)}
        return set()

    def module(self, module):
        key = (module, None)
        if key in self.cache:
            return self.cache[key]
        result = self.cache[key] = Surface()
        tree = self.trees.get(module)
        if tree is None:
            result.opaque = True
            return result
        for node in tree.body:
            if isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef)):
                result.members[node.name] = function_surface(node)
            elif isinstance(node, ast.ClassDef):
                instance = self.instance(module, node.name)
                init = instance.members.get("__init__", UNKNOWN)
                result.members[node.name] = Surface(instance.members, init.signature or ([], 0, False, False))
            elif isinstance(node, ast.Assign):
                for target in node.targets:
                    for name in names(target):
                        result.members[name] = self.value(module, node.value)
        return result

    def value(self, module, node):
        if isinstance(node, ast.Call) and isinstance(node.func, ast.Name) and (module, node.func.id) in self.classes:
            return self.instance(module, node.func.id, node.args)
        if isinstance(node, ast.Name):
            return self.module(module).members.get(node.id, UNKNOWN)
        if isinstance(node, ast.Constant):
            return SCALAR
        return UNKNOWN

    def instance(self, module, name, ctor_args=()):
        # <!-- custom: Cache before traversing self-assigned objects such as a font tag's bold variant; these declarations can refer back to their own class. (GPT-6.1-Sol) -->
        key = (module, name)
        if key in self.cache:
            result = self.cache[key]
        else:
            result = self.cache[key] = Surface()
            cls = self.classes[module, name]
            env = {}
            for item in self.trees[module].body + cls.body:
                if isinstance(item, ast.Assign):
                    for target in item.targets:
                        for var in names(target):
                            env[var] = self.literal_names(item.value, env)
            for base in cls.bases:
                if isinstance(base, ast.Name) and (module, base.id) in self.classes:
                    result.members.update(self.instance(module, base.id).members)
            for item in cls.body:
                if isinstance(item, ast.FunctionDef):
                    decorators = {decorator.id for decorator in item.decorator_list if isinstance(decorator, ast.Name)}
                    result.members[item.name] = UNKNOWN if "property" in decorators else function_surface(item, bound="staticmethod" not in decorators)
                    if item.name == "__call__":
                        result.signature = result.members[item.name].signature
                elif isinstance(item, ast.Assign):
                    for target in item.targets:
                        for member in names(target):
                            result.members[member] = self.value(module, item.value)
            for item in ast.walk(cls):
                if isinstance(item, ast.Assign):
                    for target in item.targets:
                        if isinstance(target, ast.Attribute) and isinstance(target.value, ast.Name) and target.value.id == "self":
                            result.members[target.attr] = self.value(module, item.value)
            getter = next((item for item in cls.body if isinstance(item, ast.FunctionDef) and item.name == "__getattr__"), None)
            if getter:
                dynamic = set()
                for item in ast.walk(getter):
                    if isinstance(item, (ast.Name, ast.Attribute)):
                        dynamic.update(self.literal_names(item, env))
                    if isinstance(item, ast.Call) and isinstance(item.func, ast.Name) and item.func.id == "getattr" and item.args and isinstance(item.args[0], ast.Name):
                        dynamic.update(self.enums.get(item.args[0].id, set()))
                # <!-- custom: Infer prefix-backed strict XML lookups from the implementation (for example named colors); the prefix is a runtime contract, not a checker-maintained spelling list. (GPT-6.1-Sol) -->
                if any(isinstance(item, ast.Call) and isinstance(item.func, ast.Name) and item.func.id == "getInfoTypeOrFail" for item in ast.walk(cls)):
                    for item in ast.walk(cls):
                        if isinstance(item, ast.Call) and isinstance(item.func, ast.Attribute) and item.func.attr == "startswith" and item.args and isinstance(item.args[0], ast.Constant):
                            prefix = item.args[0].value
                            if isinstance(prefix, str) and prefix.endswith("_") and prefix != "_":
                                for values in self.xml_names.values():
                                    for value in values:
                                        if value.startswith(prefix):
                                            dynamic.update((value, value[len(prefix):]))
                for item in ast.walk(getter):
                    if isinstance(item, ast.Call) and isinstance(item.func, ast.Attribute) and item.func.attr == "endswith" and item.args and isinstance(item.args[0], ast.Constant) and isinstance(item.args[0].value, str):
                        dynamic = {value for value in dynamic if value.endswith(item.args[0].value)}
                for member in dynamic:
                    result.members[member] = SCALAR
                result.opaque = not bool(dynamic)
        # <!-- custom: An art collection's XML getter is passed into its constructor; use that argument to resolve the same class separately for InterfaceArt and MovieArt. (GPT-6.1-Sol) -->
        for arg in ctor_args:
            if isinstance(arg, ast.Attribute) and arg.attr in self.xml_names:
                result = Surface(dict(result.members), result.signature)
                result.members.update({value: SCALAR for value in self.xml_names[arg.attr]})
        return result

    def export(self, module, member, seen=()):
        if (module, member) in seen:
            return None
        surface = self.module(module)
        if member in surface.members:
            return surface.members[member]
        for node in self.trees.get(module, ast.Module(body=[])).body:
            if isinstance(node, ast.Import):
                for alias in node.names:
                    if (alias.asname or alias.name.split(".")[0]) == member:
                        return self.module(alias.name)
            if isinstance(node, ast.ImportFrom) and node.module in self.trees:
                for alias in node.names:
                    if alias.asname == member or (not alias.asname and alias.name == member) or (alias.name == "*" and not member.startswith("_")):
                        original = member if alias.name == "*" else alias.name
                        found = self.export(node.module, original, seen + ((module, member),))
                        if found is not None:
                            return found
        return None

    def export_names(self, module, seen=()):
        if module in seen or module not in self.trees:
            return set()
        result = set(self.module(module).members)
        for node in self.trees[module].body:
            if isinstance(node, ast.Assign) and any(isinstance(target, ast.Name) and target.id == "__all__" for target in node.targets):
                try:
                    return set(ast.literal_eval(node.value))
                except (ValueError, TypeError):
                    pass
            if isinstance(node, ast.Import):
                result.update(alias.asname or alias.name.split(".")[0] for alias in node.names)
            elif isinstance(node, ast.ImportFrom):
                for alias in node.names:
                    if alias.name == "*":
                        result.update(self.export_names(node.module, seen + (module,)))
                    else:
                        result.add(alias.asname or alias.name)
        return {name for name in result if not name.startswith("_")}

    def imported(self, module, member=None):
        return (self.export(module, member) or UNKNOWN) if member else self.module(module)


def data_sources(root):
    xml_names = {}
    symbols = set()
    for path in (root / "Assets/XML").rglob("*.xml"):
        # <!-- custom: Only XML info records with Type identities contribute to this contract; unrelated text/schema files are validated by their own XML checks. (GPT-6.1-Sol) -->
        if not re.search(rb"<(?:[A-Za-z_]\w*:)?Type\b", path.read_bytes()):
            continue
        tree = ET.parse(path)
        for record in tree.iter():
            tag = record.tag.rsplit("}", 1)[-1]
            value = next((child.text for child in record if child.tag.rsplit("}", 1)[-1] == "Type"), None)
            if value:
                xml_names.setdefault("get" + tag, set()).add(value.strip())
    header = read(root / "CvGameCoreDLL/CvEnums.h")
    header = re.sub(r"/\*.*?\*/|//[^\n]*", "", header, flags=re.S)
    enums = {name: set(re.findall(r"\b([A-Z][A-Z0-9_]+)\s*(?:=[^,}]+)?(?=[,}])", body + "}"))
             for name, body in re.findall(r"enum\s+(\w+)\s*\{(.*?)\}", header, re.S)}
    # <!-- custom: BUG creates additional FontSymbols from symbol registrations; preserve their declared XML identities in the same enum-derived surface. (GPT-6.1-Sol) -->
    for path in (root / "Assets/Config").glob("*.xml"):
        for item in ET.parse(path).iter("symbol"):
            name = item.get("name") or item.get("id")
            if name:
                symbols.add(name.upper().replace(" ", "_") + "_CHAR")
    # <!-- custom: Info-backed symbols are registered by addSymbol loops too; derive their getter families from those loops and their names from XML, without duplicating yield/commerce lists. (GPT-6.1-Sol) -->
    provider = parse(read(root / "Assets/Python/SASVisualUtils.py"))
    for loop in ast.walk(provider):
        if not isinstance(loop, ast.For):
            continue
        calls = [item for item in ast.walk(loop) if isinstance(item, ast.Call)]
        if any(isinstance(call.func, ast.Name) and call.func.id == "addSymbol" for call in calls):
            for call in calls:
                if isinstance(call.func, ast.Attribute) and call.func.attr in xml_names:
                    symbols.update(value + "_CHAR" for value in xml_names[call.func.attr])
    enums.setdefault("FontSymbols", set()).update(symbols)
    return xml_names, enums


# <!-- custom: A changing displayed amount does not invalidate its stable glyph/art resource.
# Derive direct-lookup signatures from writes into provider caches, including inherited constructor-supplied getters, rather than copying every resource-family name into CI.
# This recognizes engine lookup origins and exact shared literal paths; it does not prove arbitrary hand-written formatting/mapping logic equivalent to a cached helper. (GPT-6.1-Sol) -->
def resource_origin(node, env):
    if isinstance(node, ast.Name):
        return env.get(node.id)
    if isinstance(node, ast.Attribute):
        key = ".".join(chain(node))
        if key in env:
            return env[key]
        base = resource_origin(node.value, env)
        return base + (node.attr,) if base else None
    if isinstance(node, ast.Call):
        origin = resource_origin(node.func, env)
        if origin and origin[:1] == ("CvPythonExtensions",) and len(origin) == 2:
            return {"CyGlobalContext": ("CyGlobalContext",), "CyGame": ("CyGlobalContext", "getGame"), "CyArtFileMgr": ("CyArtFileMgr",)}.get(origin[1])
        return origin
    return None


def resource_environment(tree):
    env = {name: (name,) for name in ("CyGlobalContext", "CyArtFileMgr")}
    env["CyGame"] = ("CyGlobalContext", "getGame")
    env["getInfoTypeOrFail"] = ("getInfoTypeOrFail",)
    for node in tree.body:
        if isinstance(node, ast.Import):
            for item in node.names:
                env[item.asname or item.name] = (item.name,)
        elif isinstance(node, (ast.FunctionDef, ast.ClassDef)):
            env[node.name] = None
        elif isinstance(node, ast.ImportFrom):
            for item in node.names:
                if item.name != "*":
                    env[item.asname or item.name] = env.get(item.name) if node.module in {"CvPythonExtensions", "SASUtils"} else (node.module, item.name)
        elif isinstance(node, ast.Assign):
            for target in node.targets:
                if isinstance(target, ast.Name):
                    env[target.id] = resource_origin(node.value, env)
    return env


class ResourceCachePolicy:
    def __init__(self, catalog):
        self.rules = {}
        self.literals = {}
        for module in PROVIDERS:
            if module not in catalog.trees:
                continue
            tree = catalog.trees[module]
            classes = {item.name: item for item in tree.body if isinstance(item, ast.ClassDef)}
            base_env = resource_environment(tree)
            for node in tree.body:
                if isinstance(node, ast.Assign) and isinstance(node.value, ast.Call) and isinstance(node.value.func, ast.Name) and node.value.func.id in classes:
                    public = next((target.id for target in node.targets if isinstance(target, ast.Name) and not target.id.startswith("_")), None)
                    if public:
                        self.instance(module, public, classes, node.value.func.id, node.value.args, base_env)
                elif isinstance(node, ast.FunctionDef):
                    env = dict(base_env)
                    for arg in node.args.args:
                        env[arg.arg] = None
                    self.method(module + "." + node.name, node, env, tree)

        # <!-- custom: The former raw-lookup tests also guarded legacy cached wrappers such as ColorUtil.keyToType.
        # Derive straightforward return-value forwarding from their source, retaining that coverage through aliases without adding a new wrapper-name blacklist for each helper. (ChatGPT-5.6-Sol + GPT-6.1-Sol) -->
        wrappers = {}
        for module, tree in catalog.trees.items():
            if module in PROVIDERS:
                continue
            for function in tree.body:
                if not isinstance(function, ast.FunctionDef):
                    continue
                env = resource_environment(tree)
                for arg in function.args.args:
                    env[arg.arg] = None
                for item in ast.walk(function):
                    if isinstance(item, ast.Assign):
                        for target in item.targets:
                            if isinstance(target, ast.Name):
                                env[target.id] = resource_origin(item.value, env)
                for item in ast.walk(function):
                    if isinstance(item, ast.Return):
                        origin = resource_origin(item.value, env)
                        if origin in self.rules:
                            rules = self.rules[origin]
                            parameters = {arg.arg for arg in function.args.args}
                            forwards_key = any(isinstance(call, ast.Call) and resource_origin(call, env) == origin and call.args and isinstance(call.args[0], ast.Name) and call.args[0].id in parameters for call in ast.walk(function))
                            if forwards_key or all(not prefixes and not literals for _accessor, prefixes, literals in rules):
                                wrappers[module, function.name] = list(rules)
        self.rules.update(wrappers)

    def instance(self, module, public, classes, name, arguments, env):
        cls = classes[name]
        state = dict(env)
        def initialize(owner, values):
            constructor = next((item for item in owner.body if isinstance(item, ast.FunctionDef) and item.name == "__init__"), None)
            if constructor is None:
                return
            local = dict(state)
            for arg, value in zip(constructor.args.args[1:], values):
                local[arg.arg] = value
            for item in ast.walk(constructor):
                if isinstance(item, ast.Call) and isinstance(item.func, ast.Attribute) and item.func.attr == "__init__" and isinstance(item.func.value, ast.Name) and item.func.value.id in classes:
                    initialize(classes[item.func.value.id], [resource_origin(arg, local) for arg in item.args[1:]])
                elif isinstance(item, ast.Assign):
                    for target in item.targets:
                        if isinstance(target, ast.Attribute) and chain(target)[:1] == ["self"]:
                            state[".".join(chain(target))] = resource_origin(item.value, local)
        initialize(cls, [resource_origin(arg, env) for arg in arguments])
        visited = set()
        def methods(owner):
            if owner.name in visited:
                return
            visited.add(owner.name)
            for base in owner.bases:
                if isinstance(base, ast.Name) and base.id in classes:
                    methods(classes[base.id])
            for item in owner.body:
                if isinstance(item, ast.FunctionDef) and not item.name.startswith("_"):
                    self.method(module + "." + public + "." + item.name, item, dict(state), owner)
                elif isinstance(item, ast.Assign):
                    for target in item.targets:
                        if isinstance(target, ast.Name) and not target.id.startswith("_") and isinstance(item.value, ast.Constant) and isinstance(item.value.value, str) and item.value.value.lower().startswith("art/"):
                            self.literals.setdefault(item.value.value.lower(), module + "." + public + "." + target.id)
        methods(cls)

    def method(self, accessor, function, env, container):
        prefixes = {item.args[0].value for item in ast.walk(container) if isinstance(item, ast.Call) and isinstance(item.func, ast.Attribute) and item.func.attr == "startswith" and item.args and isinstance(item.args[0], ast.Constant) and isinstance(item.args[0].value, str) and item.args[0].value.endswith("_") and item.args[0].value != "_"}
        literals = {item.args[0].value for item in ast.walk(container) if isinstance(item, ast.Call) and isinstance(item.func, ast.Name) and item.func.id == function.name and item.args and isinstance(item.args[0], ast.Constant) and isinstance(item.args[0].value, str)}
        for item in ast.walk(function):
            if not isinstance(item, ast.Assign):
                continue
            origin = resource_origin(item.value, env)
            for target in item.targets:
                if isinstance(target, ast.Name):
                    env[target.id] = origin
                elif isinstance(target, ast.Subscript) and origin and (origin[0] in {"CyGlobalContext", "CyArtFileMgr", "getInfoTypeOrFail"}):
                    # <!-- custom: Only a lookup whose result is stored in a returned cache is a replacement contract; incidental metadata reads such as trait descriptions are not equivalent to a cached trait icon. (GPT-6.1-Sol) -->
                    cache = ast.dump(target.value)
                    returned = any(isinstance(ret, ast.Return) and ret.value is not None and any(ast.dump(part) == cache for part in ast.walk(ret.value)) for ret in ast.walk(function))
                    if returned and (len(origin) > 1 or prefixes):
                        if origin[-1] == "getDefineINT":
                            # <!-- custom: A private generic Define reader is not the public API; derive each fixed-name wrapper and restrict the rule to its literal key, leaving unrelated/live gameplay Defines alone. (GPT-6.1-Sol) -->
                            for wrapper in container.body:
                                if not isinstance(wrapper, ast.FunctionDef) or wrapper.name.startswith("_"):
                                    continue
                                for call in ast.walk(wrapper):
                                    if isinstance(call, ast.Call) and isinstance(call.func, ast.Name) and call.func.id == function.name and call.args and isinstance(call.args[0], ast.Constant) and call.args[0].value in literals:
                                        public = accessor.rsplit(".", 1)[0] + "." + wrapper.name
                                        self.rules.setdefault(origin, []).append((public, set(), {call.args[0].value}))
                        else:
                            self.rules.setdefault(origin, []).append((accessor, prefixes if origin[0] == "getInfoTypeOrFail" else set(), set()))

    def inspect(self, tree, source):
        errors = []
        exceptions = {}
        for token in tokenize.generate_tokens(StringIO(source).readline):
            if token.type == tokenize.COMMENT and "resource-cache-direct:" in token.string:
                reason = token.string.split("resource-cache-direct:", 1)[1].split("-->", 1)[0].strip()
                if not reason or reason.startswith("("):
                    errors.append((token.start[0], "resource-cache-direct requires a concrete reason"))
                else:
                    exceptions[token.start[0]] = reason
        policy = self
        class Visitor(ast.NodeVisitor):
            def __init__(self):
                self.env = resource_environment(tree)
            def visit_FunctionDef(self, node):
                outer = self.env
                self.env = dict(outer)
                for arg in ast.walk(node.args):
                    if isinstance(arg, ast.arg):
                        self.env[arg.arg] = None
                for item in ast.walk(node):
                    if isinstance(item, ast.Name) and isinstance(item.ctx, ast.Store):
                        self.env[item.id] = None
                for statement in node.body:
                    self.visit(statement)
                self.env = outer
            visit_AsyncFunctionDef = visit_FunctionDef
            def visit_Lambda(self, node):
                outer = self.env
                self.env = dict(outer)
                for arg in ast.walk(node.args):
                    if isinstance(arg, ast.arg):
                        self.env[arg.arg] = None
                self.visit(node.body)
                self.env = outer
            def visit_Assign(self, node):
                self.visit(node.value)
                for target in node.targets:
                    if isinstance(target, ast.Name):
                        self.env[target.id] = resource_origin(node.value, self.env)
            def allowed(self, node):
                return node.lineno in exceptions or node.lineno - 1 in exceptions
            def visit_Call(self, node):
                origin = resource_origin(node, self.env)
                for accessor, prefixes, literals in policy.rules.get(origin, []):
                    literal = node.args[0].value if node.args and isinstance(node.args[0], ast.Constant) else None
                    if prefixes and (not isinstance(literal, str) or not any(literal.startswith(prefix) for prefix in prefixes)):
                        continue
                    if literals and literal not in literals:
                        continue
                    if not self.allowed(node):
                        errors.append((node.lineno, "direct resource lookup bypasses shared cache; use " + accessor + " or document resource-cache-direct: reason"))
                    break
                self.generic_visit(node)
            def visit_Constant(self, node):
                if isinstance(node.value, str) and node.value.lower() in policy.literals and not self.allowed(node):
                    errors.append((node.lineno, "duplicated shared art path; use " + policy.literals[node.value.lower()] + " or document resource-cache-direct: reason"))
        Visitor().visit(tree)
        return errors


# <!-- custom: Follow explicit module/member aliases and local-module re-exports, but only validate typed resource surfaces; arbitrary engine objects and computed return values remain outside this static check. (GPT-6.1-Sol) -->
def inspect_consumer(module, catalog):
    tree = catalog.trees[module]
    imports = bindings(tree)
    for item in ast.walk(tree):
        if isinstance(item, ast.ImportFrom) and item.module in catalog.trees and any(alias.name == "*" for alias in item.names):
            for member in catalog.export_names(item.module):
                imports.setdefault(member, (item.module, member))
    api_ids = set()
    pending = [catalog.module(owner) for owner in PROVIDERS if owner in catalog.trees]
    while pending:
        value = pending.pop()
        if value is UNKNOWN or value is SCALAR or id(value) in api_ids:
            continue
        api_ids.add(id(value))
        pending.extend(value.members.values())
    bound = set(imports)
    for node in ast.walk(tree):
        if isinstance(node, ast.Name) and isinstance(node.ctx, (ast.Store, ast.Param)):
            bound.add(node.id)
        elif isinstance(node, ast.arg):
            bound.add(node.arg)
        elif isinstance(node, (ast.FunctionDef, ast.ClassDef)):
            bound.add(node.name)
    errors = []
    scope_bindings = {}
    for item in ast.walk(tree):
        if isinstance(item, ast.ImportFrom) and item.module in PROVIDERS:
            for alias in item.names:
                if alias.name != "*" and catalog.export(item.module, alias.name) is None:
                    errors.append((item.lineno, "missing API import %s.%s" % (item.module, alias.name)))
    parents = {child: parent for parent in ast.walk(tree) for child in ast.iter_child_nodes(parent)}
    api_exports = {member for owner in PROVIDERS if owner in catalog.trees for member in catalog.module(owner).members if not member.startswith("_")}
    for node in ast.walk(tree):
        # <!-- custom: Attribute assignment still reads its owner, so an undefined namespace on the left side raises NameError too; skip bare name definitions, not attribute store targets. (GPT-6.1-Sol) -->
        if not isinstance(node, (ast.Attribute, ast.Name)) or (isinstance(node, ast.Name) and isinstance(node.ctx, ast.Store)):
            continue
        parent = parents.get(node)
        if isinstance(parent, ast.Attribute) and parent.value is node:
            continue
        parts = chain(node)
        if not parts:
            continue
        root, *members = parts
        if root not in imports:
            if root not in bound and members and members[0] in api_exports:
                errors.append((node.lineno, "unbound namespace %s for shared API member %s" % (root, members[0])))
            continue
        # <!-- custom: An imported module name can be shadowed by a parameter or local variable; do not apply its module contract to that unrelated object. (GPT-6.1-Sol) -->
        scope = parents.get(node)
        shadowed = False
        while scope is not None:
            if isinstance(scope, (ast.FunctionDef, ast.AsyncFunctionDef, ast.Lambda)):
                if scope not in scope_bindings:
                    local = {arg.arg for arg in ast.walk(scope.args) if isinstance(arg, ast.arg)}
                    local.update(item.id for item in ast.walk(scope) if isinstance(item, ast.Name) and isinstance(item.ctx, ast.Store))
                    scope_bindings[scope] = local
                local = scope_bindings[scope]
                if root in local:
                    shadowed = True
                    break
            scope = parents.get(scope)
        if shadowed:
            continue
        owner, member = imports[root]
        surface = catalog.imported(owner, member)
        relevant = owner in PROVIDERS or id(surface) in api_ids
        if relevant and member and catalog.export(owner, member) is None:
            errors.append((node.lineno, "missing API import %s.%s" % (owner, member)))
            continue
        failed = False
        for index, attr in enumerate(members):
            exported = catalog.export(owner, attr) if index == 0 and member is None else None
            if exported is not None:
                surface = exported
                relevant = relevant or id(surface) in api_ids
                continue
            if surface.opaque:
                break
            if attr not in surface.members:
                if relevant or (index == 0 and attr in api_exports):
                    errors.append((node.lineno, "missing API member %s" % ".".join(parts[:index + 2])))
                failed = True
                break
            surface = surface.members[attr]
        if failed or not relevant or not isinstance(parent, ast.Call) or parent.func is not node:
            continue
        if not surface.opaque and surface.signature is None:
            errors.append((node.lineno, "API value is not callable: %s" % ".".join(parts)))
        elif surface.signature and not any(isinstance(arg, ast.Starred) for arg in parent.args) and all(kw.arg is not None for kw in parent.keywords):
            positional, required, vararg, kwarg = surface.signature
            given = len(parent.args)
            keywords = [kw.arg for kw in parent.keywords]
            supplied = set(positional[:given]) | set(keywords)
            if (given > len(positional) and not vararg) or (not kwarg and any(key not in positional for key in keywords)) or any(key in positional[:given] for key in keywords) or any(key not in supplied for key in positional[:required]):
                errors.append((node.lineno, "invalid API arguments: %s" % ".".join(parts)))
    if module not in PROVIDERS:
        if not hasattr(catalog, "resource_cache_policy"):
            catalog.resource_cache_policy = ResourceCachePolicy(catalog)
        errors.extend(catalog.resource_cache_policy.inspect(tree, catalog.sources[module]))
    return sorted(set(errors))


# <!-- custom: Provider/data changes require all consumers; ordinary edits only require changed modules and their reverse import dependencies, including re-export chains. (GPT-6.1-Sol) -->
def affected_modules(catalog, changed):
    if changed is None or any(path.startswith(("Assets/XML/", "Assets/Config/", ".github/workflows/build/source/python_resource_api.py")) or path == "CvGameCoreDLL/CvEnums.h" or Path(path).stem in PROVIDERS for path in changed):
        return set(catalog.trees)
    selected = {Path(path).stem for path in changed if path.startswith("Assets/Python/")}
    while True:
        more = set()
        for name, tree in catalog.trees.items():
            imported = {owner for owner, _member in bindings(tree).values()}
            imported.update(node.module for node in ast.walk(tree) if isinstance(node, ast.ImportFrom))
            if imported & selected:
                more.add(name)
        if more <= selected:
            return selected
        selected.update(more)


def check(root, base_ref=None, all_files=False, event_name="local"):
    paths = {}
    for path in (root / "Assets/Python").rglob("*.py"):
        if path.stem in paths:
            raise RuntimeError("ambiguous Python module %s: %s and %s" % (path.stem, paths[path.stem], path))
        paths[path.stem] = path
    xml_names, enums = data_sources(root)
    sources = {name: read(path) for name, path in paths.items()}
    catalog = Catalog(sources, xml_names, enums)
    base = ensure_base(root, base_ref, event_name)
    changed = None if all_files or base is None else run_git(root, "diff", "--name-only", base, "--").stdout.decode().splitlines()
    selected = affected_modules(catalog, changed)
    errors = []
    for name in sorted(selected & paths.keys()):
        for line, message in inspect_consumer(name, catalog):
            errors.append("%s:%d: %s" % (paths[name].relative_to(root).as_posix(), line, message))
    return errors, len(selected & paths.keys())


def main():
    parser = argparse.ArgumentParser(description="Check source-derived shared Python resource API contracts without importing Civ4.")
    parser.add_argument("--repo-root", type=Path, default=ROOT)
    parser.add_argument("--base-ref")
    parser.add_argument("--event-name", default="local")
    parser.add_argument("--all", action="store_true")
    args = parser.parse_args()
    try:
        errors, count = check(args.repo_root, args.base_ref, args.all, args.event_name)
    except (OSError, RuntimeError, SyntaxError, ET.ParseError) as error:
        print("FAIL Python resource API setup: %s" % error)
        return 1
    print(("FAIL" if errors else "PASS") + " Python resource API (%d consumers checked)" % count)
    for error in errors:
        print("  - " + error)
    return bool(errors)


if __name__ == "__main__":
    sys.exit(main())
