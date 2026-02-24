import registrationForm from "./forms/register.js"

document.addEventListener("htmx:afterRequest", () => {
    registrationForm()
});





function main() {
    document.addEventListener("DOMContentLoaded", () => {
        console.log("Hello world");

        let canvas = document.getElementById("game");
        canvas.style.backgroundColor = "blue";
        canvas.width = 300;
        canvas.height = 150;


    });

}

main()

