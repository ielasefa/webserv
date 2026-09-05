#!/usr/bin/env python3
import html
import os
import sys
from urllib.parse import parse_qs


def read_request_body():
    method = os.environ.get("REQUEST_METHOD", "")
    content_length = os.environ.get("CONTENT_LENGTH", "0")

    if method != "POST":
        return "", 0

    try:
        length = int(content_length)
    except ValueError:
        length = 0

    if length <= 0:
        return "", length

    return sys.stdin.read(length), length


method = os.environ.get("REQUEST_METHOD", "")
content_type = os.environ.get("CONTENT_TYPE", "")
content_length = os.environ.get("CONTENT_LENGTH", "0")
body, body_size = read_request_body()
fields = parse_qs(body, keep_blank_values=True)

print("Content-Type: text/html; charset=utf-8")
print("")
print("<!doctype html>")
print("<html lang='en'>")
print("<head>")
print("<meta charset='utf-8'>")
print("<meta name='viewport' content='width=device-width, initial-scale=1'>")
print("<title>CGI POST test</title>")
print("<style>")
print("  :root { color-scheme: dark; --bg: #0a0d17; --panel: rgba(17, 24, 42, 0.92); --panel-soft: rgba(13, 18, 32, 0.78); --border: rgba(255,255,255,0.1); --text: #f5f7fb; --muted: rgba(245,247,251,0.7); --accent: #38bdf8; --accent-soft: rgba(56, 189, 248, 0.14); --good: #5eead4; --radius: 22px; }")
print("  * { box-sizing: border-box; }")
print("  body { margin: 0; min-height: 100vh; font-family: Inter, ui-sans-serif, system-ui, sans-serif; color: var(--text); background: radial-gradient(circle at 15% 20%, rgba(56, 189, 248, 0.18), transparent 24%), radial-gradient(circle at 82% 12%, rgba(94, 234, 212, 0.12), transparent 22%), linear-gradient(135deg, #0a0d17 0%, #0f1324 55%, #0a0d17 100%); }")
print("  .shell { width: min(1120px, calc(100% - 32px)); margin: 0 auto; padding: 28px 0 40px; }")
print("  .hero, .panel { border: 1px solid var(--border); border-radius: var(--radius); background: var(--panel); box-shadow: 0 24px 60px rgba(0,0,0,0.32); backdrop-filter: blur(18px); }")
print("  .hero { padding: 28px; position: relative; overflow: hidden; }")
print("  .hero::before { content: ''; position: absolute; inset: -60px -40px auto auto; width: 220px; height: 220px; border-radius: 999px; background: radial-gradient(circle, rgba(56, 189, 248, 0.2), transparent 68%); pointer-events: none; }")
print("  .eyebrow { display: inline-flex; padding: 8px 12px; border: 1px solid rgba(255,255,255,0.16); border-radius: 999px; color: var(--muted); font-size: 12px; letter-spacing: .08em; text-transform: uppercase; background: rgba(255,255,255,0.04); }")
print("  h1 { margin: 14px 0 10px; font-size: clamp(2.2rem, 5vw, 4.4rem); line-height: .98; letter-spacing: -0.05em; }")
print("  p { margin: 0; color: var(--muted); line-height: 1.7; }")
print("  .grid { display: grid; grid-template-columns: repeat(2, minmax(0, 1fr)); gap: 18px; margin-top: 18px; }")
print("  .panel { padding: 22px; }")
print("  .panel h2 { margin: 0 0 12px; font-size: 1.05rem; }")
print("  .meta { display: grid; gap: 10px; }")
print("  .row { display: flex; justify-content: space-between; gap: 12px; padding: 10px 12px; border-radius: 14px; background: rgba(255,255,255,0.04); border: 1px solid rgba(255,255,255,0.06); }")
print("  .row strong { font-size: .92rem; }")
print("  .row span, .body { color: var(--muted); }")
print("  .body { margin-top: 14px; padding: 14px; border-radius: 16px; border: 1px dashed rgba(255,255,255,0.16); background: rgba(255,255,255,0.03); white-space: pre-wrap; word-break: break-word; }")
print("  .chips { display: flex; flex-wrap: wrap; gap: 8px; margin-top: 12px; }")
print("  .chip { display: inline-flex; align-items: center; min-height: 34px; padding: 0 12px; border-radius: 999px; border: 1px solid rgba(56,189,248,0.25); background: rgba(56,189,248,0.12); color: #bfdbfe; font-size: 13px; }")
print("  .actions { display: flex; flex-wrap: wrap; gap: 10px; margin-top: 18px; }")
print("  .button { display: inline-flex; align-items: center; justify-content: center; min-height: 42px; padding: 0 16px; border-radius: 999px; border: 1px solid transparent; text-decoration: none; font-weight: 700; color: white; background: linear-gradient(135deg, rgba(56, 189, 248, 0.95), rgba(59, 130, 246, 0.92)); }")
print("  .button.secondary { background: rgba(255,255,255,0.04); border-color: rgba(255,255,255,0.16); }")
print("  .success { color: var(--good); }")
print("  @media (max-width: 820px) { .grid { grid-template-columns: 1fr; } .shell { width: min(100% - 20px, 1120px); padding: 16px 0 28px; } .hero, .panel { border-radius: 20px; } }")
print("</style>")
print("</head>")
print("<body>")
print("  <main class='shell'>")
print("    <section class='hero'>")
print("      <span class='eyebrow'>CGI POST test</span>")
print("      <h1>Request received successfully.</h1>")
print("      <p>The form reached the CGI endpoint and the server passed the request body through correctly.</p>")
print("      <div class='actions'>")
print("        <a class='button' href='/'>Back to test lab</a>")
print("        <a class='button secondary' href='/cgi-bin/post.py'>Reload CGI test</a>")
print("      </div>")
print("    </section>")
print("    <section class='grid'>")
print("      <article class='panel'>")
print("        <h2>Request details</h2>")
print("        <div class='meta'>")
print("          <div class='row'><strong>Method</strong><span>{}</span></div>".format(html.escape(method or 'UNKNOWN'))) 
print("          <div class='row'><strong>Content-Type</strong><span>{}</span></div>".format(html.escape(content_type or 'unknown')))
print("          <div class='row'><strong>Content-Length</strong><span>{}</span></div>".format(html.escape(content_length or '0')))
print("          <div class='row'><strong>Body size read</strong><span>{}</span></div>".format(body_size))
print("        </div>")
print("      </article>")
print("      <article class='panel'>")
print("        <h2>Parsed fields</h2>")
if fields:
    print("        <div class='chips'>")
    for key, values in fields.items():
        for value in values:
            print("          <span class='chip'><strong>{}</strong>&nbsp;=&nbsp;{}</span>".format(html.escape(key), html.escape(value)))
    print("        </div>")
else:
    print("        <p>No form fields were parsed from the request body.</p>")
print("        <div class='body'>" + html.escape(body or '(empty body)') + "</div>")
print("      </article>")
print("    </section>")
print("  </main>")
print("</body>")
print("</html>")
