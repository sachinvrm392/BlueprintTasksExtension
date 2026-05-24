import { MCPClient } from "mcp-client";
import path from "path";
import { fileURLToPath } from "url";

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

const client = new MCPClient({
  name: "Practice MCP Inspector",
  version: "0.1.0",
});

async function main() {
  console.log("Connecting MCP Inspector to the local Practice MCP stub...");

  await client.connect({
    type: "stdio",
    command: "/workspaces/BlueprintTasksExtension/.venv/bin/fastmcp",
    args: ["run", path.join(__dirname, "fastmcp_server.py"), "--transport", "stdio", "--no-banner"],
    cwd: __dirname,
  });

  console.log("Connected. Listing tools and resources...");

  const tools = await client.getAllTools();
  const resources = await client.getAllResources();
  const prompts = await client.getAllPrompts();

  console.log("Tools:", tools.slice(0, 10));
  console.log("Resources:", resources);
  console.log("Prompts:", prompts);

  const pingResult = await client.ping();
  console.log("Ping result:", pingResult);
}

main().catch((error) => {
  console.error("MCP Inspector connection failed:", error);
  process.exit(1);
});
