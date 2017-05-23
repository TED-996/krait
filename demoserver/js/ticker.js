function Ticker() {
    this.ticker = $("#ticker");
    this.ticker_add = function(msg, para_class) {
        var para = $("<p>").text(msg);
        if (para_class){
            para.attr("class", para_class);
        }
        if (this.ticker.children().length > 10){
            this.ticker.children().last().remove();
        }
        this.ticker.prepend(para);
    };
}