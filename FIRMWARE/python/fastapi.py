from fastapi import FastAPI, HTTPException
from fastapi.responses import JSONResponse
from pydantic import BaseModel

app = FastAPI(
    title="3ESE REST API",
    description="Version FASTAPI du TP REST",
    version="1.0.0",
)

# ======================
#   Base sentence
# ======================
welcome = "Welcome to 3ESE API!"


# ======================
#   Models for JSON input
# ======================
class Sentence(BaseModel):
    text: str

class InsertText(BaseModel):
    text: str

class PatchChar(BaseModel):
    char: str


# ======================
#       /status
# ======================
@app.get("/api/status")
def get_status():
    return {"status": "ok", "message": "API is running"}


# ======================
#   CRUD on /welcome/
# ======================
@app.get("/api/welcome/")
def get_sentence():
    return {"text": welcome}


@app.post("/api/welcome/", status_code=202)
def update_sentence(payload: Sentence):
    global welcome
    welcome = payload.text
    return JSONResponse(status_code=202, content={"message": "Sentence updated"})


@app.delete("/api/welcome/", status_code=204)
def delete_sentence():
    global welcome
    welcome = ""
    return JSONResponse(status_code=204, content=None)


# ======================
# CRUD on /welcome/{index}
# ======================
@app.get("/api/welcome/{index}")
def get_letter(index: int):
    if index < 0 or index >= len(welcome):
        raise HTTPException(status_code=400, detail="index out of range")
    return {"index": index, "val": welcome[index]}


@app.put("/api/welcome/{index}")
def insert_text(index: int, payload: InsertText):
    global welcome

    if index < 0 or index > len(welcome):
        raise HTTPException(status_code=400, detail="index out of range")

    welcome = welcome[:index] + payload.text + welcome[index:]
    return {"text": welcome}


@app.patch("/api/welcome/{index}")
def patch_letter(index: int, payload: PatchChar):
    global welcome

    if index < 0 or index >= len(welcome):
        raise HTTPException(status_code=400, detail="index out of range")

    if len(payload.char) != 1:
        raise HTTPException(status_code=400, detail="char must be exactly 1 character")

    welcome = welcome[:index] + payload.char + welcome[index + 1:]
    return {"text": welcome}


@app.delete("/api/welcome/{index}")
def delete_letter(index: int):
    global welcome

    if index < 0 or index >= len(welcome):
        raise HTTPException(status_code=400, detail="index out of range")

    welcome = welcome[:index] + welcome[index + 1:]
    return {"text": welcome}

