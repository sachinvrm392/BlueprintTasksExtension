"""FastMCP prototype integration for Practice MCP."""

import json
from pathlib import Path
from typing import Any, Dict, List

import dummy_playwright_tools

BASE_DIR = Path(__file__).resolve().parent


class FastMCP:
    def __init__(self):
        self.resources = self._load_json(BASE_DIR / "resources.json")
        self.data = self._load_json(BASE_DIR / "data.json")
        self.prompts = self._load_json(BASE_DIR / "prompts.json")
        self.tools = dummy_playwright_tools.TOOL_REGISTRY

    def _load_json(self, path: Path) -> Any:
        if not path.exists():
            raise FileNotFoundError(f"Missing MCP definition file: {path}")
        with path.open("r", encoding="utf-8") as handle:
            return json.load(handle)

    def start(self) -> str:
        print("FastMCP stub started.")
        return "Ok started"

    def list_tools(self) -> List[str]:
        return sorted(self.tools.keys())

    def list_resources(self) -> List[Dict[str, Any]]:
        return self.resources

    def list_data(self) -> List[Dict[str, Any]]:
        return self.data

    def list_prompts(self) -> Dict[str, str]:
        return self.prompts

    def invoke_tool(self, name: str, url: str) -> str:
        print(f"Invoking tool {name} with URL: {url}")
        return dummy_playwright_tools.run_tool(name, url)


def main() -> None:
    mcp = FastMCP()
    mcp.start()
    print("Available tools:", ", ".join(mcp.list_tools()[:5]), "...")
    print("Resources loaded:", len(mcp.list_resources()))
    print("Data entries loaded:", len(mcp.list_data()))
    print("Prompts loaded:", len(mcp.list_prompts()))
    result = mcp.invoke_tool("browser_to_url_01", "https://example.com")
    print("Result:", result)


if __name__ == "__main__":
    main()
