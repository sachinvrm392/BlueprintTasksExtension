from pathlib import Path

from fastmcp.resources.base import Resource
from fastmcp.prompts.base import Prompt
from fastmcp.server import FastMCP

from dummy_playwright_tools import TOOL_REGISTRY

BASE_DIR = Path(__file__).resolve().parent

mcp = FastMCP(
    name="BlueprintTasksExtension MCP Server",
    version="0.1.0",
    instructions="FastMCP server exposing dummy Playwright tools, resources, and prompts for MCP Inspector integration.",
)

# Register dummy browser_to_url tools
for tool in TOOL_REGISTRY.values():
    mcp.add_tool(tool)

# Resources

def repo_overview() -> str:
    return (
        "BlueprintTasksExtension is an Unreal Engine plugin extension that provides dialog, quest, and task graph nodes. "
        "This Practice MCP stub exposes dummy Playwright tools and metadata for MCP Inspector testing."
    )


def plugin_architecture() -> str:
    return (
        "The plugin includes BT_Dialogue, BT_Quests, and Blueprint task graph functionality. "
        "Core systems are split into Private and Public source files, with tasks, decorators, and data assets."
    )


def mcp_plan_description() -> str:
    return (
        "This MCP server is a prototype for connecting the repository to an MCP Inspector client. "
        "It exposes tools, resources, data, and prompts through standard MCP APIs."
    )

mcp.add_resource(
    Resource.from_function(
        repo_overview,
        uri="resource://repo-overview",
        name="Repository Overview",
        description="High-level description of the BlueprintTasksExtension repository.",
        mime_type="text/plain",
    )
)

mcp.add_resource(
    Resource.from_function(
        plugin_architecture,
        uri="resource://plugin-architecture",
        name="Plugin Architecture",
        description="Summary of the plugin architecture and modules.",
        mime_type="text/plain",
    )
)

mcp.add_resource(
    Resource.from_function(
        mcp_plan_description,
        uri="resource://mcp-plan",
        name="MCP Plan",
        description="Description of the Practice MCP plan and integration.",
        mime_type="text/plain",
    )
)

# Prompts

def initialize_session() -> str:
    return "Initialize the MCP session and confirm that the FastMCP stub is ready."


def list_tools_prompt() -> str:
    return "List the available dummy browser_to_url tools that are exposed by this MCP server."


def execute_tool_prompt() -> str:
    return "Execute a browser_to_url tool by name with a URL argument and return the result."


def describe_resource_prompt() -> str:
    return "Describe the requested resource by id and provide its purpose."


def query_data_prompt() -> str:
    return "Query the sample data definitions and return the matching payload or metadata."

mcp.add_prompt(
    Prompt.from_function(
        initialize_session,
        name="initialize_session",
        description="Check that the FastMCP stub is ready for MCP Inspector.",
    )
)

mcp.add_prompt(
    Prompt.from_function(
        list_tools_prompt,
        name="list_tools",
        description="List the available Playwright-style browser_to_url tools.",
    )
)

mcp.add_prompt(
    Prompt.from_function(
        execute_tool_prompt,
        name="execute_tool",
        description="Provide information on running a tool with a URL argument.",
    )
)

mcp.add_prompt(
    Prompt.from_function(
        describe_resource_prompt,
        name="describe_resource",
        description="Describe a resource and what it contains.",
    )
)

mcp.add_prompt(
    Prompt.from_function(
        query_data_prompt,
        name="query_data",
        description="Return example data payload information.",
    )
)
