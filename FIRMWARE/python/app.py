from flask import Flask, jsonify, render_template, request
from api import api_bp


def create_app():
    app = Flask(__name__)

    # Enregistrement du blueprint sous le préfixe /api
    app.register_blueprint(api_bp, url_prefix="/api")

    # === Gestion globale des erreurs ===

    @app.errorhandler(404)
    def not_found(error):
        """
        404 :
        - si c'est une route /api/... -> JSON
        - sinon -> page HTML templates/page_not_found.html
        """
        if request.path.startswith("/api/"):
            return jsonify({
                "error": "Not Found",
                "message": "The requested resource was not found."
            }), 404

        return render_template("page_not_found.html"), 404

    @app.errorhandler(500)
    def internal_error(error):
        """
        500 :
        Réponse JSON pour les erreurs serveur.
        """
        return jsonify({
            "error": "Internal Server Error",
            "message": "An unexpected error occurred on the server."
        }), 500

    return app


# Instance globale pour Flask
app = create_app()

if __name__ == "__main__":
    # Lancement direct (optionnel, pour python app.py)
    app.run(host="0.0.0.0", port=5000, debug=True)

