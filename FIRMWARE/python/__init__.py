from flask import Blueprint, jsonify

# Création du blueprint "api"
api_bp = Blueprint("api", __name__)

# Texte de base
welcome = "Welcome to 3ESE API!"


@api_bp.route("/status")
def status():
    """
    Route de statut simple : /api/status
    """
    return jsonify({
        "status": "ok",
        "message": "API is running",
    })


@api_bp.route("/welcome/")
def api_welcome():
    """
    /api/welcome/
    Retourne le message complet en texte simple.
    """
    return welcome


@api_bp.route("/welcome/<int:index>")
def api_welcome_index(index):
    """
    /api/welcome/<index>
    Retourne un caractère précis sous forme JSON.
    Gestion propre de l'erreur si index hors limites.
    """
    try:
        return jsonify({
            "index": index,
            "val": welcome[index]
        })
    except IndexError:
        return jsonify({
            "error": "index out of range",
            "max_index": len(welcome) - 1
        }), 400


@api_bp.route("/welcome/all")
def api_welcome_all():
    """
    /api/welcome/all
    Retourne tous les caractères avec leur index.
    """
    letters = [
        {"index": i, "val": ch}
        for i, ch in enumerate(welcome)
    ]
    return jsonify({
        "text": welcome,
        "letters": letters
    })

